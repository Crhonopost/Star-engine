#include <GL/glew.h>
#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/rendering.hpp>
#include <engine/include/camera.hpp>
#include <engine/include/geometryHelper.hpp>
#include <engine/include/animation.hpp>

#include <iostream>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/Importer.hpp>

void renderQuad();

void Render::update(glm::mat4 &view) {    
    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<Drawable>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        auto& program = *ecs.GetComponent<CustomProgram>(entity).programPtr;
        
        float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
        glUseProgram(program.programID);
        program.beforeRender();
        
        glm::mat4 model = transform.getModelMatrix();

        program.renderTextures();

        program.updateViewMatrix(view);
        program.updateModelMatrix(model);

        drawable.draw(distanceToCam);
        program.afterRender();
    }
}

PBR* PBRrender::pbrProgPtr = nullptr;

void PBRrender::initPBR() {
    if (!pbrProgPtr) {
        pbrProgPtr = new PBR();
    }
}

void PBRrender::setupMaps(){
    int current = Texture::getAvailableActivationInt();

    PBR &pbrProg = *pbrProgPtr;
    glUseProgram(pbrProg.programID);

    GLuint camLoc = glGetUniformLocation(pbrProg.programID, "camPos");
    glm::vec3 C = Camera::getInstance().getPosition();
    glUniform3f(camLoc, C.x, C.y, C.z);

    ////////// irradiance map
    GLuint irrLoc = glGetUniformLocation(pbrProg.programID, "irradianceMap");
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMapID);
    glUniform1i(irrLoc, current);

    ////////// prefilter map
    GLuint prefiLoc = glGetUniformLocation(pbrProg.programID, "prefilterMap");
    current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMapID);
    glUniform1i(prefiLoc, current);
    ////////// brdf lut map
    GLuint brdfLoc = glGetUniformLocation(pbrProg.programID, "brdfLUTMap");
    current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_2D, mBrdfLUTID);
    glUniform1i(brdfLoc, current);
}

void PBRrender::update(glm::mat4 &view){
    setupMaps();

    PBR &pbrProg = *pbrProgPtr;
    pbrProg.beforeRender();
    pbrProg.updateViewMatrix(view);

    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<Drawable>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        auto& material = ecs.GetComponent<Material>(entity);

        pbrProg.updateMaterial(material);
        
        float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
        glm::mat4 model = transform.getModelMatrix();

        pbrProg.renderTextures();
        pbrProg.updateModelMatrix(model);

        drawable.draw(distanceToCam);
    }
    pbrProg.afterRender();
}


void AnimatedPBRrender::update(glm::mat4 &view, float deltaTime){
    setupMaps();

    PBR &pbrProg = *pbrProgPtr;
    pbrProg.beforeRender();
    pbrProg.updateViewMatrix(view);

    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<AnimatedDrawable>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        auto& material = ecs.GetComponent<Material>(entity);
        
        pbrProg.updateMaterial(material);
        
        float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
        glm::mat4 model = transform.getModelMatrix();
        
        pbrProg.renderTextures();
        pbrProg.updateModelMatrix(model);
        
        if(drawable.playing){
            drawable.animation.addDeltaTime(deltaTime);
        }
        std::vector<glm::mat4> inMatrices, outMatrices;
        
        drawable.animation.getPose(drawable.bones, inMatrices);
        
        CalculateAnimationPose(drawable.bones, inMatrices, outMatrices);
        for (int i = 0; i < outMatrices.size(); i++) {
            std::string uniformName = "bones[" + std::to_string(i) + "]";
            GLuint mLoc = glGetUniformLocation(pbrProg.programID, uniformName.c_str());
            glUniformMatrix4fv(mLoc, 1, GL_FALSE, &outMatrices[i][0][0]);
        }

        drawable.draw(distanceToCam);
    }
    pbrProg.afterRender();
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


std::vector<Texture*> loadMaterialTextures(aiMaterial* material,
                                           aiTextureType type,
                                           const char* directory,
                                           const aiScene* scene)
{
    std::vector<Texture*> texturesOut;
    std::string dirStr = directory ? directory : "";
    if (!dirStr.empty() && dirStr.back() != '/' && dirStr.back() != '\\')
        dirStr += '/';

    for (unsigned int i = 0; i < material->GetTextureCount(type); ++i) {
        aiString str;
        material->GetTexture(type, i, &str);
        std::string texKey;
        Texture* texPtr = nullptr;

        if (str.C_Str()[0] == '*') {
            int texIndex = std::atoi(str.C_Str() + 1);
            if (texIndex >= 0 && texIndex < static_cast<int>(scene->mNumTextures)) {
                aiTexture* atex = scene->mTextures[texIndex];
                texKey = std::string(scene->mMeshes[0]->mName.C_Str()) + std::string("embedded_") + std::to_string(texIndex);

                if (atex->mHeight) {
                    texPtr = &Texture::loadTextureFromMemory(
                        reinterpret_cast<unsigned char*>(atex->pcData),
                        0,
                        atex->mWidth,
                        atex->mHeight,
                        4,
                        texKey
                    );
                } else {
                    texPtr = &Texture::loadTextureFromMemory(
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
            std::string fullPath = dirStr + str.C_Str();
            texKey = fullPath;
            texPtr = &Texture::loadTexture(fullPath.c_str());
        }

        if (texPtr) {
            texturesOut.push_back(texPtr);
        }
    }
    return texturesOut;
}


void extractMaterial(Material &mat, aiMesh *mesh, const aiScene *scene, const char *directory){
    if (mesh->mMaterialIndex >= 0){
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<Texture*> albedoMap = loadMaterialTextures(material, aiTextureType_BASE_COLOR, directory, scene);
        if (!albedoMap.empty()){
            mat.albedoTex = albedoMap[0];
            mat.albedoTex->visible = true;
        } 
        else mat.albedoTex = &Texture::emptyTexture;
        
        std::vector<Texture*> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, directory, scene);
        if (!specularMaps.empty()) {
            mat.metallicTex = specularMaps[0];
            mat.metallicTex->visible = true;
        }
        else mat.metallicTex = &Texture::emptyTexture;
        
        std::vector<Texture*> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, directory, scene);
        if (normalMaps.empty()) normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, directory, scene);
        if (!normalMaps.empty()){
            mat.normalTex = normalMaps[0];
            mat.normalTex->visible = true;
        }
        else mat.normalTex = &Texture::emptyTexture;
        
        std::vector<Texture*> roughnessMaps = loadMaterialTextures(material, aiTextureType_SHININESS, directory, scene);
        if (!roughnessMaps.empty()){
            mat.roughnessTex = roughnessMaps[0];
            mat.roughnessTex->visible = true;
        }
        else mat.roughnessTex = &Texture::emptyTexture;
        
        std::vector<Texture*> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, directory, scene);
        if (!aoMaps.empty()){
            mat.aoTex = aoMaps[0];
            mat.aoTex->visible = true;
        } 
        else mat.aoTex = &Texture::emptyTexture;
    }
}


void Render::loadSimpleMesh(char *directory, char *fileName, Drawable &res, Material &mat){
    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    
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

        extractMaterial(mat, mesh, scene, directory);
    }


    res.indexCount = indices.size();

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

    res.init(vertex_buffer_data, indices);
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

void AnimatedPBRrender::loadMesh(char *directory, char *fileName, AnimatedDrawable &res, Material &mat){
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

        extractMaterial(mat, mesh, scene, directory);


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


            // Vérification cohérence offsetMatrix <-> globalBindPoseMatrix⁻¹
            std::map<std::string, aiMatrix4x4> globalTransforms;
            std::function<void(aiNode*, aiMatrix4x4)> computeGlobalTransform;
            computeGlobalTransform = [&](aiNode* node, aiMatrix4x4 parentTransform) {
                aiMatrix4x4 global = parentTransform * node->mTransformation;
                globalTransforms[node->mName.C_Str()] = global;
                for (unsigned int i = 0; i < node->mNumChildren; ++i) {
                    computeGlobalTransform(node->mChildren[i], global);
                }
            };
            computeGlobalTransform(scene->mRootNode, aiMatrix4x4());

            for (unsigned int j = 0; j < mesh->mNumBones; ++j) {
                aiBone* bone = mesh->mBones[j];
                std::string boneName = bone->mName.C_Str();

                auto it = globalTransforms.find(boneName);
                if (it == globalTransforms.end()) {
                    std::cerr << "Warning: Bone name " << boneName << " not found in node hierarchy.\n";
                    continue;
                }

                aiMatrix4x4 globalBind = it->second;
                aiMatrix4x4 inverseGlobalBind = globalBind;
                inverseGlobalBind.Inverse();

                aiMatrix4x4 offset = bone->mOffsetMatrix;

                float epsilon = 1e-3f;
                bool equal = true;
                for (int row = 0; row < 4 && equal; ++row)
                    for (int col = 0; col < 4 && equal; ++col)
                        if (fabs(offset[row][col] - inverseGlobalBind[row][col]) > epsilon)
                            equal = false;

                if (!equal) {
                    std::cerr << "Mismatch in offset matrix for bone: " << boneName << "\n";
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


    res.indexCount = indices.size();

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

    res.init(vertex_buffer_data, indices);
}


void LightRender::update(){
    //TODO: update as a batch https://gamedev.stackexchange.com/questions/179539/how-to-set-the-value-of-each-index-in-a-uniform-array
    int associatedLight = 0;
    glUseProgram(PBRrender::pbrProgPtr->programID);
    for (const auto& entity : mEntities) {
        auto& light = ecs.GetComponent<Light>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        
        PBRrender::pbrProgPtr->updateLightPosition(associatedLight, transform.getLocalPosition());
        PBRrender::pbrProgPtr->updateLightColor(associatedLight, light.color);

        associatedLight ++;
    }
    
    PBRrender::pbrProgPtr->updateLightCount(associatedLight);
}

void LightRender::computeLights(PBRrender *pbr, InfosRender &infosRender, Skybox *skyboxProgPtr){
    CubemapRender sceneCubemapRender(512);
    int activationInt;

    int lightIdx = 0;
    for(auto &lightEntity : mEntities){
        Light &light = ecs.GetComponent<Light>(lightEntity);
        Transform &lightTransform = ecs.GetComponent<Transform>(lightEntity);

        sceneCubemapRender.renderInfosFromPoint(lightTransform.getGlobalPosition(), infosRender, 1);
        activationInt = sceneCubemapRender.unwrapOctaProj(light.depthID, 512, skyboxProgPtr);

        glUseProgram(pbr->pbrProgPtr->programID);
        auto strIdx = "[" + std::to_string(lightIdx ++) + "]";
        light.shaderLoc = glGetUniformLocation(pbr->pbrProgPtr->programID, ("lightDepthMaps" + strIdx).c_str());
    }
}


ProbeManager::ProbeManager(): prog(){}

void ProbeManager::initProbes(Render *render, PBRrender *pbr, InfosRender &infosRender, Skybox *skyboxProgPtr){
    prog.use();
    prog.beforeRender();

    CubemapRender sceneCubemapRender(512);

    glm::vec3 startPos{0,0,0};
    int width, height, depth;
    width = height = depth = 2;
    glm::vec3 gridSize{2.f};

    glUniform3f(prog.probeStepLoc, gridSize.x, gridSize.y, gridSize.z);
    glUniform3i(prog.probeCountsLoc, width, height, depth);
    glUniform3f(prog.probeStartPositionLoc, startPos.x, startPos.y, startPos.z);
    glUniform1f(prog.lowResolutionDownsampleFactorLoc, 2.f);

    for(int x=0; x<width; x++){
        for(int y=0; y<height; y++){
            for(int z=0; z<depth; z++){
                int index = x + y * width + z * width * height;
                auto strIdx = "[" + std::to_string(index) + "]";

                GLuint radianceProbeGridLoc = glGetUniformLocation(prog.programID, ("radianceProbeGrid" + strIdx).c_str());
                GLuint normalProbeGridLoc = glGetUniformLocation(prog.programID, ("normalProbeGrid" + strIdx).c_str());
                GLuint distanceProbeGridLoc = glGetUniformLocation(prog.programID, ("distanceProbeGrid" + strIdx).c_str());
                GLuint lowResolutionDistanceProbeGridLoc = glGetUniformLocation(prog.programID, ("lowResolutionDistanceProbeGrid" + strIdx).c_str());
                GLuint irradianceProbeGridLoc = glGetUniformLocation(prog.programID, ("irradianceProbeGrid" + strIdx).c_str());
                

                glm::vec3 position = glm::vec3(width * gridSize.x, height * gridSize.y, depth * gridSize.z) + startPos;
                sceneCubemapRender.renderFromPoint(position, render, pbr);
            
                GLuint octa;
                int activationInt = sceneCubemapRender.unwrapOctaProj(octa, 512, (Skybox*) skyboxProgPtr);
                textureIDs.push_back(octa);
                glUniform1i(radianceProbeGridLoc, activationInt);


                GLuint normalOcta;
                sceneCubemapRender.renderInfosFromPoint(position, infosRender, 0);
                activationInt = sceneCubemapRender.unwrapOctaProj(normalOcta, 512, (Skybox*) skyboxProgPtr);
                textureIDs.push_back(normalOcta);
                glUniform1i(normalProbeGridLoc, activationInt);


                GLuint depthOcta;
                sceneCubemapRender.renderInfosFromPoint(position, infosRender, 1);
                activationInt = sceneCubemapRender.unwrapOctaProj(depthOcta, 512, (Skybox*) skyboxProgPtr);
                textureIDs.push_back(depthOcta);
                glUniform1i(distanceProbeGridLoc, activationInt);


                GLuint lowDepthOcta;
                sceneCubemapRender.renderInfosFromPoint(position, infosRender, 1);
                activationInt = sceneCubemapRender.unwrapOctaProj(lowDepthOcta, 256, (Skybox*) skyboxProgPtr);
                textureIDs.push_back(lowDepthOcta);
                glUniform1i(lowResolutionDistanceProbeGridLoc, activationInt);
            }
        }
    }
}

void ProbeManager::clear(){
    for(auto &texID : textureIDs){
        glDeleteTextures(1, &texID);
    }
}


InfosRender::InfosRender(): infoProgram("shaders/infos/vertex.glsl", "shaders/infos/fragment.glsl"){};


void InfosRender::update(glm::mat4 &view, glm::mat4 &projection, int mode){
    infoProgram.use();
    infoProgram.beforeRender();
    infoProgram.updateViewMatrix(view);
    infoProgram.updateProjectionMatrix(projection);
    
    GLuint renderLocation = glGetUniformLocation(infoProgram.programID, "renderMode");
    glUniform1i(renderLocation, mode);
    
    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<Drawable>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        
        glm::mat4 model = transform.getModelMatrix();
        infoProgram.updateModelMatrix(model);

        drawable.draw(-1);
    }

    infoProgram.afterRender();
}

GLuint InfosRender::renderOnFrame(glm::mat4 &view, glm::mat4 &projection, int width, int height, int mode){
    if(mode == 1){
        glClearColor(0,0,0,1);
    }
    GLuint resTexID;

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufffer;
    glGenRenderbuffers(1, &depthBufffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufffer);


    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cerr << "Framebuffer not complete!" << std::endl;
    }    

    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    glViewport(0,0, width, height);


    glGenTextures(1, &resTexID);
    glActiveTexture(GL_TEXTURE0 + Texture::getAvailableActivationInt());
    glBindTexture(GL_TEXTURE_2D, resTexID);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,                    // ou GL_RGBA selon ton besoin
        width,
        height,
        0,
        GL_RGB,                    // même format ici
        GL_UNSIGNED_BYTE,
        nullptr                    // pas de données, juste allocation
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resTexID, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update(view, projection, mode);

    save_PPM_file(width, height, "../pictures/info.ppm");

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);

    return resTexID;
}

CubemapRender::CubemapRender(int res): cubemap(res), cubeMesh(Render::generateCube(10, 2, true)){
    orientations[0] = {1,0,0};
    orientations[1] = {-1,0,0};
    ups[0] = ups[1] = {0,-1,0};
    
    orientations[2] = {0,1,0};
    orientations[3] = {0,-1,0};
    ups[2] = {0,0,1};
    ups[3] = {0,0,-1};
    
    orientations[4] = {0,0,1};
    orientations[5] = {0,0,-1};
    ups[4] = ups[5] = {0,-1,0};

    projection = glm::perspective(glm::radians(90.0f), 1.f, 0.1f, 1000.f);
    
}



void CubemapRender::applyPrefilter(Program* prefilterProg, Cubemap prefilterMap) {
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap.textureID);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    GLuint fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    prefilterProg->use();
    GLuint skyLoc = glGetUniformLocation(prefilterProg->programID, "environmentMap");
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.textureID);
    glUniform1i(skyLoc, current);
    
    prefilterProg->beforeRender();
    prefilterProg->updateProjectionMatrix(projection);
    prefilterProg->updateModelMatrix(glm::mat4(1.0f));

    const unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipSize = static_cast<unsigned int>(prefilterMap.resolution * std::pow(0.5f, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);

        float roughness = float(mip) / float(maxMipLevels - 1);
        prefilterProg->setFloat("roughness", roughness);

        
        for (int face = 0; face < 6; ++face) {
            
            glm::mat4 view = glm::lookAt(glm::vec3(0.0f), orientations[face], ups[face]);
            prefilterProg->updateViewMatrix(view);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                prefilterMap.textureID,
                mip
            );
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cubeMesh.draw(-1);
        }
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap.textureID);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    prefilterProg->afterRender();
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}


GLuint CubemapRender::TwoDLUT(Program* brdfProg) {
    GLuint brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    // Configuration de la texture
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Création locale du FBO/RBO
    GLuint fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    // Rendu
    GLint oldVp[4];
    glGetIntegerv(GL_VIEWPORT, oldVp);
    glViewport(0, 0, 512, 512);
    
    brdfProg->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    // Restauration
    glViewport(oldVp[0], oldVp[1], oldVp[2], oldVp[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);

    return brdfLUTTexture;
}



void CubemapRender::applyFilter(Program *filterProg, Cubemap target){
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufffer;
    glGenRenderbuffers(1, &depthBufffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, target.resolution, target.resolution);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufffer);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cerr << "[ERROR]Framebuffer not complete!" << std::endl;
    }    

    // Save current viewport dimmensions and prepare it for cubemap faces
    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    glViewport(0,0, target.resolution, target.resolution);

    filterProg->use();

    GLuint skyLoc = glGetUniformLocation(filterProg->programID, "skybox");
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.textureID);
    glUniform1i(skyLoc, current);

    filterProg->beforeRender();
    filterProg->updateProjectionMatrix(projection);
    filterProg->updateModelMatrix(glm::mat4(1));

    for(int i = 0; i < 6; i++){
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               target.textureID, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(glm::vec3(0), orientations[i], ups[i]);
        filterProg->updateViewMatrix(view);

        cubeMesh.draw(-1);
    }

    glm::mat4 camProj = Camera::getInstance().getP();
    filterProg->updateProjectionMatrix(camProj);
    filterProg->afterRender();

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);
}


void CubemapRender::renderInfosFromPoint(glm::vec3 point, InfosRender &infosRender, int mode){
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufffer;
    glGenRenderbuffers(1, &depthBufffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemap.resolution, cubemap.resolution);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufffer);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cerr << "Framebuffer not complete!" << std::endl;
    }    

    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    glViewport(0,0, cubemap.resolution, cubemap.resolution);

    if(mode == 1) glClearColor(0,0,0,1);

    for(int i=0; i<6; i++){
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap.textureID, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto dir = orientations[i];
        
        glm::mat4 view = glm::lookAt(point, point + dir, ups[i]);
        infosRender.update(view, projection, mode);
    }

    save_PPM_file(cubemap.resolution, cubemap.resolution, "../pictures/verif.ppm");

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);
}

void CubemapRender::renderFromPoint(glm::vec3 point, Render *render, PBRrender *pbr){
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufffer;
    glGenRenderbuffers(1, &depthBufffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemap.resolution, cubemap.resolution);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufffer);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cerr << "Framebuffer not complete!" << std::endl;
    }    

    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    glViewport(0,0, cubemap.resolution, cubemap.resolution);

    for(auto &prog:Program::programs){
        prog->updateProjectionMatrix(projection);
    }
    for(int i=0; i<6; i++){
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap.textureID, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto dir = orientations[i];
        
        glm::mat4 view = glm::lookAt(point, point + dir, ups[i]);
        
        render->update(view);
        pbr->update(view);

        int a = Texture::getAvailableActivationInt();
        save_PPM_file(cubemap.resolution, cubemap.resolution, "../pictures/" + std::to_string(a) + ".ppm");
    }

    glm::mat4 camProjection = Camera::getInstance().getP();
    for(auto &prog:Program::programs){
        prog->updateProjectionMatrix(camProjection);
    }

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);
}


int CubemapRender::unwrapOctaProj(GLuint &textureID, int resolution, Skybox *skyboxProgPtr){
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufffer;
    glGenRenderbuffers(1, &depthBufffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufffer);


    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cerr << "Framebuffer not complete!" << std::endl;
    }    

    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    glViewport(0,0, cubemap.resolution, cubemap.resolution);


    skyboxProgPtr->use();
    auto identity = glm::mat4(1);
    skyboxProgPtr->setProjectionOcta(true);
    auto save = skyboxProgPtr->cubemap;
    skyboxProgPtr->cubemap = cubemap;
    skyboxProgPtr->updateModelMatrix(identity);
    skyboxProgPtr->updateProjectionMatrix(identity);
    // glm::mat4 view = glm::lookAt(glm::vec3(0), {0,0,1}, {0,1,0});
    skyboxProgPtr->updateViewMatrix(identity);
    skyboxProgPtr->beforeRender();

    Drawable plane = Render::generatePlane(2, 2, true);

    // Generate texture
    glGenTextures(1, &textureID);
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,                    // ou GL_RGBA selon ton besoin
        resolution,
        resolution,
        0,
        GL_RGB,                    // même format ici
        GL_UNSIGNED_BYTE,
        nullptr                    // pas de données, juste allocation
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    plane.draw(-1);


    glm::mat4 camProj = Camera::getInstance().getP();
    skyboxProgPtr->updateProjectionMatrix(camProj);
    skyboxProgPtr->setProjectionOcta(false);
    skyboxProgPtr->afterRender();
    skyboxProgPtr->cubemap = save;

    save_PPM_file(resolution, resolution, "../pictures/octaProj" + std::to_string(Texture::getAvailableActivationInt()) + ".ppm");

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);

    return current;
}


Drawable Render::generateSphere(float radius){
    Drawable res;

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    const int latitudeBands = 20;
    const int longitudeBands = 20;

    for (int lat = 0; lat <= latitudeBands; ++lat) {
        float theta = lat * glm::pi<float>() / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= longitudeBands; ++lon) {
            float phi = lon * 2 * glm::pi<float>() / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            glm::vec3 vertex = radius * glm::vec3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            indexed_vertices.push_back(vertex);
            normal.push_back(glm::normalize(vertex));

            tex_coords.push_back(glm::vec2(
                (float) lon / (float) longitudeBands, 
                (float) lat / (float) latitudeBands
            ));
        }
    }

    for (int lat = 0; lat < latitudeBands; ++lat) {
        for (int lon = 0; lon < longitudeBands; ++lon) {
            int first = (lat * (longitudeBands + 1)) + lon;
            int second = first + longitudeBands + 1;

            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(first);
            
            indices.push_back(first + 1);
            indices.push_back(second + 1);
            indices.push_back(second);
        }
    }

    res.indexCount = indices.size();

    std::vector<Vertex> vertex_buffer_data;

    for(int i=0; i<indexed_vertices.size(); i++){
        Vertex v;
        
        v.position = indexed_vertices[i];
        v.normal = normal[i];
        v.texCoord = tex_coords[i];
    

        v.boneIndices = {0,0,0,0};
        v.boneWeights = {0,0,0,0};
        
        vertex_buffer_data.push_back(v);
    }

    res.init(vertex_buffer_data, indices);

    return res;
}

Drawable Render::generatePlane(float sideLength, int nbOfVerticesSide, bool front){
    Drawable res;

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    float edgeLength = sideLength / (nbOfVerticesSide - 1.);

    glm::vec3 offsetNFront(sideLength / 2., 0, sideLength / 2.);
    glm::vec3 offsetFront(sideLength / 2., sideLength / 2., 0);

    for(size_t i=0; i<nbOfVerticesSide; i++){
        for(size_t j=0; j<nbOfVerticesSide; j++){
            glm::vec3 vertexPos;
            if(!front){
                vertexPos = edgeLength * glm::vec3(i, 0, j) - offsetNFront;
            } else {
                vertexPos = edgeLength * glm::vec3(i, j, 0) - offsetFront;
            }

            indexed_vertices.push_back(vertexPos);
            normal.push_back(glm::vec3(0.f,1.f,0.f));
            
            glm::vec2 v_coords(
                1. - (double) i / (double) (nbOfVerticesSide - 1),
                (double) j / (double) (nbOfVerticesSide - 1));
            tex_coords.push_back(v_coords);
        }
    }

    for(size_t i=0; i<nbOfVerticesSide-1; i++){
        for(size_t j=1; j<nbOfVerticesSide; j++){
            indices.push_back(i * nbOfVerticesSide + j - 1);
            indices.push_back(i * nbOfVerticesSide + j);
            indices.push_back((i + 1) * nbOfVerticesSide + j - 1);

            indices.push_back((i + 1) * nbOfVerticesSide + j - 1);
            indices.push_back(i * nbOfVerticesSide + j);
            indices.push_back((i + 1) * nbOfVerticesSide + j);        
        }
    }

    res.indexCount = indices.size();
    std::vector<Vertex> vertex_buffer_data;
    
    for(int i=0; i<indexed_vertices.size(); i++){
        Vertex v;
        
        v.position = indexed_vertices[i];
        v.normal = normal[i];
        v.texCoord = tex_coords[i];
    

        v.boneIndices = {0,0,0,0};
        v.boneWeights = {0,0,0,0};
        
        vertex_buffer_data.push_back(v);
    }

    res.init(vertex_buffer_data, indices);

    return res;
}


/**
 * Generates a cube mesh with specified parameters
 * @param sideLength Length of each side of the cube
 * @param verticesPerSide Number of vertices along each edge of a face
 * @param inward If true, normals face inward and winding order is reversed
 * @return Drawable object containing the cube mesh
 */
Drawable Render::generateCube(float sideLength, int verticesPerSide, bool inward) {
    Drawable result;

    if (sideLength <= 0.0f) throw std::invalid_argument("Side length must be positive");
    if (verticesPerSide < 2) throw std::invalid_argument("At least 2 vertices per side are required");

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;

    const float edgeIncrement = sideLength / (verticesPerSide - 1);
    const float halfLength = sideLength / 2.0f;

    struct Face {
        glm::vec3 origin;
        glm::vec3 uDir;
        glm::vec3 vDir;
        glm::vec3 normal;
    };

    // Faces with correct outward-pointing normals
    const std::vector<Face> faces = {
        // +Y (top)
        {{-halfLength,  halfLength,  halfLength}, {edgeIncrement, 0, 0}, {0, 0, -edgeIncrement}, {0, 1, 0}},
        // -Y (bottom)
        {{-halfLength, -halfLength, -halfLength}, {edgeIncrement, 0, 0}, {0, 0, edgeIncrement}, {0, -1, 0}},
        // +X (right)
        {{ halfLength, -halfLength, -halfLength}, {0, edgeIncrement, 0}, {0, 0, edgeIncrement}, {1, 0, 0}},
        // -X (left)
        {{-halfLength, -halfLength,  halfLength}, {0, edgeIncrement, 0}, {0, 0, -edgeIncrement}, {-1, 0, 0}},
        // +Z (front)
        {{-halfLength, -halfLength,  halfLength}, {edgeIncrement, 0, 0}, {0, edgeIncrement, 0}, {0, 0, 1}},
        // -Z (back)
        {{ halfLength, -halfLength, -halfLength}, {-edgeIncrement, 0, 0}, {0, edgeIncrement, 0}, {0, 0, -1}}
    };

    int vertexOffset = 0;
    const float texCoordStep = 1.0f / (verticesPerSide - 1);

    // Reserve memory for better performance
    const int verticesPerFace = verticesPerSide * verticesPerSide;
    const int indicesPerFace = 2 * 3 * (verticesPerSide - 1) * (verticesPerSide - 1);
    vertices.reserve(6 * verticesPerFace);
    texCoords.reserve(6 * verticesPerFace);
    normals.reserve(6 * verticesPerFace);
    indices.reserve(6 * indicesPerFace);

    for (const auto& face : faces) {
        // Generate vertices and texture coordinates
        for (int i = 0; i < verticesPerSide; ++i) {
            for (int j = 0; j < verticesPerSide; ++j) {
                vertices.push_back(face.origin + static_cast<float>(i)*face.uDir + static_cast<float>(j)*face.vDir);
                normals.push_back(inward ? -face.normal : face.normal);
                texCoords.emplace_back(i * texCoordStep, j * texCoordStep);
            }
        }

        // Generate indices with correct winding order
        for (int i = 0; i < verticesPerSide - 1; ++i) {
            for (int j = 0; j < verticesPerSide - 1; ++j) {
                const int a = vertexOffset + i * verticesPerSide + j;
                const int b = a + 1;
                const int c = a + verticesPerSide;
                const int d = c + 1;

                if (!inward) {
                    // Clockwise winding
                    indices.insert(indices.end(), {a, c, b});
                    indices.insert(indices.end(), {b, c, d});
                } else {
                    // Counter-clockwise winding
                    indices.insert(indices.end(), {a, b, c});
                    indices.insert(indices.end(), {b, d, c});
                }
            }
        }

        vertexOffset += verticesPerFace;
    }

    std::vector<Vertex> vertexBuffer;
    vertexBuffer.reserve(vertices.size() * 8);
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        Vertex v;
        v.position = {vertices[i].x, vertices[i].y, vertices[i].z};
        v.texCoord = {texCoords[i].x, texCoords[i].y};
        v.normal = {normals[i].x, normals[i].y, normals[i].z};
        
        v.boneIndices = {0,0,0,0};
        v.boneWeights = {0,0,0,0};

        vertexBuffer.push_back(v);
    }

    result.indexCount = static_cast<unsigned int>(indices.size());
    result.init(vertexBuffer, indices);
    
    return result;
}


void CameraSystem::update(){
    for (const auto& entity : mEntities) {
        auto& cam = ecs.GetComponent<CameraComponent>(entity);
        if(cam.needActivation && !cam.activated){
            cam.activated = true;
            cams.push(entity);
        }
    }

    while(!ecs.GetComponent<CameraComponent>(cams.front()).needActivation){
        cams.pop();
    }

    if(cams.size() == 0){
        std::cerr << "no valid camera detected!!";
        return;
    } else {
        auto& cam = ecs.GetComponent<CameraComponent>(cams.front());        
        auto& transform = ecs.GetComponent<Transform>(cams.front());
        
        Camera::getInstance().camera_position = transform.getGlobalPosition();
        
        glm::vec3 forward = glm::mat3(transform.getModelMatrix()) * glm::vec3(0,0,1);
        forward.x = -forward.x;
        forward.y = -forward.y;
        
        Camera::getInstance().camera_target = Camera::getInstance().camera_position + glm::normalize(forward);
    }
}


void CustomSystem::update(float deltaTime){
    for(auto const& entity: mEntities){
        auto& behavior = ecs.GetComponent<CustomBehavior>(entity);

        behavior.update(deltaTime);
    }
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}