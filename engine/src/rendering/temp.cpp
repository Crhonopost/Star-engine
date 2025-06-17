#include <engine/include/rendering/rendering.hpp>
#include <engine/include/ecs/ecsManager.hpp>


#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/Importer.hpp>
#include <engine/include/API/ResourceManagement/ResourceManager.hpp>


void SingleMesh::init(std::vector<Vertex> &vertices, std::vector<short unsigned int> &indices) {
    nbOfIndices = indices.size();

    glGenVertexArrays(1,&VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
    
    int vertexSize = 12 * sizeof(float) + 4 * sizeof(int);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertexSize, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertexSize, (void*)(5 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, vertexSize, (void*)(8 * sizeof(float)));

    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, vertexSize, (void*)(12 * sizeof(float)));


    glBindVertexArray(0);
}

void SingleMesh::draw() {
    glBindVertexArray(VAO);
    
    glDrawElements(
                GL_TRIANGLES,      // mode
                nbOfIndices,
                GL_UNSIGNED_SHORT,   // type
                (void*)0           // element array buffer offset
                );

    glBindVertexArray(0);
}

bool SingleMesh::load(const std::string &name) {
    // This function is not used in SingleMesh, but can be implemented if needed.
    std::cerr << "SingleMesh does not support loading from file." << std::endl;
    return false;
}

void MultiMesh::draw() {
    if (subMeshes.empty()) {
        std::cerr << "No submeshes to draw!" << std::endl;
        return;
    }

    for(const auto& subMesh : subMeshes) {
        if (subMesh) {
            subMesh->draw();
        } else {
            std::cerr << "Submesh is null!" << std::endl;
        }
    }
}

std::vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* material,
                                           aiTextureType type,
                                           const aiScene* scene)
{
    std::vector<std::shared_ptr<Texture>> texturesOut;

    for (unsigned int i = 0; i < material->GetTextureCount(type); ++i) {
        aiString str;
        material->GetTexture(type, i, &str);
        std::string texKey;
        std::shared_ptr<Texture> texPtr = std::make_shared<Texture>();

        if (str.C_Str()[0] == '*') {
            int texIndex = std::atoi(str.C_Str() + 1);
            if (texIndex >= 0 && texIndex < static_cast<int>(scene->mNumTextures)) {
                aiTexture* atex = scene->mTextures[texIndex];
                texKey = std::string(scene->mMeshes[0]->mName.C_Str()) + std::string("embedded_") + std::to_string(texIndex);

                if (atex->mHeight) {
                    texPtr->loadTextureFromData(
                        reinterpret_cast<unsigned char*>(atex->pcData),
                        0,
                        atex->mWidth,
                        atex->mHeight,
                        4,
                        texKey
                    );
                } else {
                    texPtr->loadTextureFromData(
                        reinterpret_cast<unsigned char*>(atex->pcData),
                        atex->mWidth,
                        0,
                        0,
                        0,
                        texKey
                    );
                }
            }
        } else {
            std::cerr << "Error loading texture outside of mesh file: " << str.C_Str() << std::endl;
            // std::string fullPath = dirStr + str.C_Str();
            // texKey = fullPath;
            // texPtr = TextureManager::load(fullPath.c_str());
        }

        if (texPtr) {
            texturesOut.push_back(texPtr);
        }
    }
    return texturesOut;
}

void extractMaterial(Material &mat, aiMesh *mesh, const aiScene *scene){
    if (mesh->mMaterialIndex >= 0){
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        auto albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, scene);
        if (!albedoMaps.empty()){
            mat.albedoTex = albedoMaps[0];
            mat.albedoTex->visible = true;
        } 
        else mat.albedoTex = Texture::emptyTexture;
        
        auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, scene);
        if (!specularMaps.empty()) {
            mat.metallicTex = specularMaps[0];
            mat.metallicTex->visible = true;
        }
        else mat.metallicTex = Texture::emptyTexture;
        
        auto normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, scene);
        if (normalMaps.empty()) normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, scene);
        if (!normalMaps.empty()){
            mat.normalTex = normalMaps[0];
            mat.normalTex->visible = true;
        }
        else mat.normalTex = Texture::emptyTexture;
        
        auto roughnessMaps = loadMaterialTextures(material, aiTextureType_SHININESS, scene);
        if (!roughnessMaps.empty()){
            mat.roughnessTex = roughnessMaps[0];
            mat.roughnessTex->visible = true;
        }
        else mat.roughnessTex = Texture::emptyTexture;
        
        auto aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, scene);
        if (!aoMaps.empty()){
            mat.aoTex = aoMaps[0];
            mat.aoTex->visible = true;
        } 
        else mat.aoTex = Texture::emptyTexture;
    }
}



bool MultiMesh::load(const std::string &name) {    
    Assimp::Importer importer;
    std::string filePath = name;
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_FlipUVs); // si besoin

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
    }

    subMeshes.resize(scene->mNumMeshes);
    
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        subMeshes[i] = std::make_shared<SingleMesh>();
        
        std::vector<unsigned short> indices;
        std::vector<glm::vec3> indexed_vertices;
        std::vector<glm::vec2> tex_coords;
        std::vector<glm::vec3> normal;
        aiMesh* mesh = scene->mMeshes[i];
    
        for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
            aiFace& face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; ++k) {
                indices.push_back(face.mIndices[k]);  // Ajoute l'indice
            }
        }
    
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            aiVector3D vertex = mesh->mVertices[j];
            indexed_vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));  // Ajoute le vertex
        }
    
        if (mesh->HasNormals()) {
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                aiVector3D normal_vec = mesh->mNormals[j];
                normal.push_back(glm::vec3(normal_vec.x, normal_vec.y, normal_vec.z));  // Ajoute la normale
            }
        }
    
        if (mesh->HasTextureCoords(0)) {
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                aiVector3D tex_coord = mesh->mTextureCoords[0][j];
                tex_coords.push_back(glm::vec2(tex_coord.x, tex_coord.y));  // Ajoute la coordonnée de texture
            }
        }

        const std::string materialName = mesh->mMaterialIndex < scene->mNumMaterials ? scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str() : "default";
        subMeshes[i]->material = std::make_shared<Material>();
        extractMaterial(*subMeshes[i]->material.get(), mesh, scene);
        
        
        std::vector<Vertex> vertex_buffer_data;
        for (int i = 0; i < indexed_vertices.size(); ++i) {
            Vertex v;
            v.position = indexed_vertices[i];
            v.normal = normal[i];
            v.texCoord = tex_coords[i];
            
            v.boneWeights = glm::vec4(0.0f);
            v.boneIndices = glm::ivec4(0);
            
            vertex_buffer_data.push_back(v);
        }
    

        subMeshes[i]->init(vertex_buffer_data, indices);
    }

    return true;
}

void RenderServer::init(ecsManager& ecsRef){
    ecs = &ecsRef;

    pbrRenderSystem = ecs->RegisterSystem<SystemPBR>();
    animatedPbrRenderSystem = ecs->RegisterSystem<SystemAnimatedPBR>();
    lightRenderSystem = ecs->RegisterSystem<SystemLight>();

    pbrRenderSystem->initPBR();

    Signature pbrRenderSignature;
    pbrRenderSignature.set(ecs->GetComponentType<Transform>());
    pbrRenderSignature.set(ecs->GetComponentType<Drawable>());
    ecs->SetSystemSignature<SystemPBR>(pbrRenderSignature);

    Signature animatedPbrSignature;
    animatedPbrSignature.set(ecs->GetComponentType<Transform>());
    animatedPbrSignature.set(ecs->GetComponentType<AnimatedDrawable>());
    ecs->SetSystemSignature<SystemAnimatedPBR>(animatedPbrSignature);

    Signature lightSignature;
    lightSignature.set(ecs->GetComponentType<Transform>());
    lightSignature.set(ecs->GetComponentType<Light>());
    ecs->SetSystemSignature<SystemLight>(lightSignature);
}

void RenderServer::reset() {
    environmentRender = std::make_shared<EnvironmentRender>(ecs);
    environmentRender->renderSkybox(glm::mat4(1.0f));
    lightRenderSystem->update();

    CubemapRender sceneCubemapRender(256);
    // Render scene into a cubemap
    sceneCubemapRender.renderFromPoint({0,0,0}, *pbrRenderSystem, *environmentRender);
    
    ///////////////////////// diffuse irradiance
    auto irradianceShader = std::make_unique<ProgIrradiance>();        
    Cubemap irradianceMap(32);
    
    // Apply shader onto skybox
    sceneCubemapRender.applyFilter(irradianceShader.get(), irradianceMap);
    pbrRenderSystem->setIrradianceMap(irradianceMap.textureID);
    
    ///////////////////////// diffuse irradiance END


    ///////////////////////// specular IBL
    auto prefilterShader = std::make_unique<ProgPrefilter>();
    auto brdfShader =  std::make_unique<ProgBRDF>();
    
    Cubemap prefilterMap(128);
    sceneCubemapRender.applyPrefilter(prefilterShader.get(),prefilterMap);
    GLuint brdfLUTTEXID = sceneCubemapRender.TwoDLUT(brdfShader.get());

    pbrRenderSystem->setPrefilterMap(prefilterMap.textureID);
    pbrRenderSystem->setBrdfLUT(brdfLUTTEXID);
    ///////////////////////// specular IBL END    
}

void RenderServer::update(glm::mat4 &view, float deltaTime) {
    lightRenderSystem->update();
    environmentRender->renderSkybox(view);
    pbrRenderSystem->update(view);
    animatedPbrRenderSystem->update(view, deltaTime);
}



void ExtractBoneHierarchy(aiNode* node, 
    const std::unordered_map<std::string, int>& boneMap,
    std::vector<Bone>& bones,
    int parentIdx = -1) 
{
    std::string nodeName(node->mName.data);

    if (boneMap.count(nodeName)) {
        int boneIdx = boneMap.at(nodeName);
        bones[boneIdx].parentIdx = parentIdx;
        bones[boneIdx].localTransform = convertToGlm(node->mTransformation);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ExtractBoneHierarchy(node->mChildren[i], boneMap, bones, 
            boneMap.count(nodeName) ? boneMap.at(nodeName) : parentIdx);
    }
}

int reorganizeBones(std::vector<Bone>& bones, std::vector<int>& newIndices, int currentIdx) {
    for (int i = 0; i < newIndices.size(); i++) {
        if (newIndices[i] == currentIdx) {
            return i;
        }
    }
    
    Bone& bone = bones[currentIdx];
    if (bone.parentIdx != -1) {
        bone.parentIdx = reorganizeBones(bones, newIndices, bone.parentIdx);
    }
    
    newIndices.push_back(currentIdx);
    return newIndices.size() - 1;
}



void ApplyMirroredRotationToBones(aiNode* node) {
    if (node == nullptr) return;

    bool hasLeft, hasRight;
    hasLeft = hasRight = false;
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        aiNode* childNode = node->mChildren[i];

        // Exemple de convention : Si le nom du nœud contient "left" ou "right"
        bool isLeft = std::string(childNode->mName.C_Str()).find("Left") != std::string::npos;
        bool isRight = std::string(childNode->mName.C_Str()).find("Right") != std::string::npos;
        
        hasLeft = hasLeft || isLeft;
        hasRight = hasRight || isRight;

        ApplyMirroredRotationToBones(childNode);
    }

    if(hasLeft && hasRight){
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            aiNode* childNode = node->mChildren[i];
            
            bool isLeft = std::string(childNode->mName.C_Str()).find("Left") != std::string::npos;
            bool isRight = std::string(childNode->mName.C_Str()).find("Right") != std::string::npos;

            if (isLeft || isRight) {
                aiMatrix4x4 rotationMatrix;
                aiMatrix4x4::RotationZ(AI_MATH_PI, rotationMatrix);

                childNode->mTransformation = childNode->mTransformation * rotationMatrix;
            }
        }
    }
}

void SystemAnimatedPBR::loadMesh(char *directory, char *fileName, AnimatedDrawable &res, Material &mat){
    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    std::vector<glm::vec4> bones_weights;
    std::vector<glm::ivec4> bones_indices;
    
    Assimp::Importer importer;
    std::string filePath = std::string(directory) + fileName;
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_FlipUVs); // si besoin

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
    }
    
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
    
        for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
            aiFace& face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; ++k) {
                indices.push_back(face.mIndices[k]);  // Ajoute l'indice
            }
        }
    
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            aiVector3D vertex = mesh->mVertices[j];
            indexed_vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));  // Ajoute le vertex
        }
    
        if (mesh->HasNormals()) {
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                aiVector3D normal_vec = mesh->mNormals[j];
                normal.push_back(glm::vec3(normal_vec.x, normal_vec.y, normal_vec.z));  // Ajoute la normale
            }
        }
    
        if (mesh->HasTextureCoords(0)) {
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                aiVector3D tex_coord = mesh->mTextureCoords[0][j];
                tex_coords.push_back(glm::vec2(tex_coord.x, tex_coord.y));  // Ajoute la coordonnée de texture
            }
        }

        extractMaterial(mat, mesh, scene);


        if( mesh->HasBones()){
            // TODO: find a better solution for left and right
            // ApplyMirroredRotationToBones(scene->mRootNode);

            std::unordered_map<std::string, int> boneNameToIdx;
            for (unsigned int j = 0; j < mesh->mNumBones; j++) {
                aiBone* aiBone = mesh->mBones[j];
                std::string boneName(aiBone->mName.data);
                
                if (boneNameToIdx.find(boneName) == boneNameToIdx.end()) {
                    int idx = boneNameToIdx.size();
                    boneNameToIdx[boneName] = idx;
                    
                    Bone b;
                    b.name = boneName;
                    res.bones.push_back(b);
                }
            }

            std::vector<Bone> tempBones(res.bones.size());
            ExtractBoneHierarchy(scene->mRootNode, boneNameToIdx, res.bones);

            std::vector<std::vector<std::pair<int, float>>> vertexInfluences(mesh->mNumVertices);
            
            // res.bones[0].localTransform = glm::scale(res.bones[0].localTransform, glm::vec3(1.f/100.f));            
            // res.bones[0].localTransform *= glm::mat4(
            //     1, 0,  0, 0,
            //     0, 0, 1, 0,
            //     0, 1,  0, 0,
            //     0, 0,  0, 1
            // );

            for (unsigned int j = 0; j < mesh->mNumBones; ++j) {
                auto bone = mesh->mBones[j];
                int boneIndex = boneNameToIdx[bone->mName.C_Str()];


                res.bones[boneIndex].offsetMatrix = convertToGlm(bone->mOffsetMatrix);
                for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                    uint vertexId = bone->mWeights[w].mVertexId;
                    float weight = bone->mWeights[w].mWeight;
                    vertexInfluences[vertexId].emplace_back(boneIndex, weight);
                }
            }

            for (auto& influences : vertexInfluences) {
                std::sort(influences.begin(), influences.end(), [](auto& a, auto& b) {
                    return a.second > b.second;
                });
            
                glm::ivec4 bi(0);
                glm::vec4 bw(0.0f);
            
                for (size_t c = 0; c < std::min<size_t>(4, influences.size()); ++c) {
                    bi[c] = influences[c].first;
                    bw[c] = influences[c].second;
                }
            
                bones_indices.push_back(bi);
                bones_weights.push_back(glm::normalize(bw));
            }
        }
    }

    Animation anim;
    if(scene->HasAnimations()){
        auto aiAnim = scene->mAnimations[0];
        anim.duration = aiAnim->mDuration;
        anim.ticksPerSecond = aiAnim->mTicksPerSecond;

        for(uint channelIdx=0; channelIdx<aiAnim->mNumChannels; channelIdx ++){
            auto aiBoneAnim = aiAnim->mChannels[channelIdx];
            
            BoneAnimation boneAnim;
            boneAnim.nodeName = aiBoneAnim->mNodeName.C_Str();
            for(int posIdx=0; posIdx<aiBoneAnim->mNumPositionKeys; posIdx++){
                auto data  = aiBoneAnim->mPositionKeys[posIdx];
                glm::vec3 position(data.mValue[0], data.mValue[1], data.mValue[2]);
                // glm::vec3 position(data.mValue[0], data.mValue[2], -data.mValue[1]);
                position /= 100.f;
                boneAnim.positionKeys.push_back({data.mTime, position});

            }

            
            auto firstRot = aiBoneAnim->mRotationKeys[0];
            glm::quat firstRotGLM = {firstRot.mValue.w, firstRot.mValue.x, firstRot.mValue.y, firstRot.mValue.z};

            glm::quat tPoseInverse = glm::conjugate(firstRotGLM);

            for(int rotIdx=0; rotIdx<aiBoneAnim->mNumRotationKeys; rotIdx++){
                auto data  = aiBoneAnim->mRotationKeys[rotIdx];
                // boneAnim.rotationKeys.push_back({data.mTime, {data.mValue.w, data.mValue.x, data.mValue.y, data.mValue.z}});
                
                glm::quat rotGLM = {data.mValue.w, data.mValue.x, data.mValue.y, data.mValue.z};
                boneAnim.rotationKeys.push_back({data.mTime, tPoseInverse * rotGLM});
            }

            for(int scaleIdx=0; scaleIdx<aiBoneAnim->mNumScalingKeys; scaleIdx++){
                auto data  = aiBoneAnim->mScalingKeys[scaleIdx];
                boneAnim.scaleKeys.push_back({data.mTime, {data.mValue.x, data.mValue.y, data.mValue.z}});
            }

            anim.boneAnimations.insert({boneAnim.nodeName, boneAnim});
        }
    }
    res.animation = anim;

    
    std::vector<Vertex> vertex_buffer_data;
    for (int i = 0; i < indexed_vertices.size(); ++i) {
        Vertex v;
        v.position = indexed_vertices[i];
        v.normal = normal[i];
        v.texCoord = tex_coords[i];
        
        if(scene->HasAnimations()){
            v.boneWeights = bones_weights[i];
            v.boneIndices = bones_indices[i];
        } else {
            v.boneWeights = glm::vec4(0.0f);
            v.boneIndices = glm::ivec4(0);
        }
        
        vertex_buffer_data.push_back(v);
    }
    
    std::shared_ptr<SingleMesh> singleMesh = std::make_shared<SingleMesh>();
    singleMesh->init(vertex_buffer_data, indices);
    res.mesh = singleMesh;
}


EnvironmentRender::EnvironmentRender(ecsManager* ecs) {
    Cubemap skyboxMap({
    "../assets/images/cubemaps/galaxy_space/left.jpg",
    "../assets/images/cubemaps/galaxy_space/right.jpg",
    "../assets/images/cubemaps/galaxy_space/top.jpg",
    "../assets/images/cubemaps/galaxy_space/bot.jpg",
    "../assets/images/cubemaps/galaxy_space/back.jpg",
    "../assets/images/cubemaps/galaxy_space/front.jpg"});
    
    skyboxProg = std::make_unique<ProgSkybox>(skyboxMap);
    skyboxDraw = Drawable();
    skyboxDraw.mesh = std::make_shared<MultiMesh>();
    skyboxDraw.mesh = MeshHelper::generateCube(9999, 2, true);
    skyboxProg->updateProjectionMatrix(glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10000.0f));
}

void EnvironmentRender::renderSkybox(const glm::mat4& view) {
    skyboxProg->beforeRender();
    skyboxProg->updateViewMatrix(view);
    skyboxDraw.draw(-1.f);
    skyboxProg->afterRender();
}
