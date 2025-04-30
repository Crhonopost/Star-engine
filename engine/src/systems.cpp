#include <GL/glew.h>
#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/rendering.hpp>
#include <engine/include/camera.hpp>
#include <iostream>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/Importer.hpp>



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

void PBRrender::update(glm::mat4 &view){
    int current = Texture::getAvailableActivationInt();

    PBR &pbrProg = *pbrProgPtr;
    glUseProgram(pbrProg.programID);
    GLuint irrLoc = glGetUniformLocation(pbrProg.programID, "irradianceMap");
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMapID);
    glUniform1i(irrLoc, current);


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


CubemapRender::CubemapRender(int res): cubemap(res){
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

    projection = glm::perspective(glm::radians(90.0f), 1.f, 0.1f, 100.f);
    
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

    // Skybox cube (find a way to make it global)
    auto cube = Render::generateCube(10, 2, true);

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

        cube.draw(-1);
    }

    glm::mat4 camProj = Camera::getInstance().getP();
    filterProg->updateProjectionMatrix(camProj);
    filterProg->afterRender();

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
    }

    glm::mat4 camProjection = Camera::getInstance().getP();
    for(auto &prog:Program::programs){
        prog->updateProjectionMatrix(camProjection);
    }

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);
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

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    res.indexCount = indices.size();

    std::vector<float> vertex_buffer_data;
    
    for(int i=0; i<indexed_vertices.size(); i++){
        glm::vec3 vertex = indexed_vertices[i];
        glm::vec3 n = normal[i];

        vertex_buffer_data.push_back(vertex.x);
        vertex_buffer_data.push_back(vertex.y);
        vertex_buffer_data.push_back(vertex.z);
    
        vertex_buffer_data.push_back(tex_coords[i].x);
        vertex_buffer_data.push_back(tex_coords[i].y);

        vertex_buffer_data.push_back(n.x);
        vertex_buffer_data.push_back(n.y);
        vertex_buffer_data.push_back(n.z);

    }

    res.init(vertex_buffer_data, indices);

    return res;
}

Drawable Render::generatePlane(float sideLength, int nbOfVerticesSide){
    Drawable res;

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    float edgeLength = sideLength / (nbOfVerticesSide - 1.);

    glm::vec3 offset(sideLength / 2., 0, sideLength / 2.);

    glm::vec3 min(999,999,999);
    glm::vec3 max(-999,-999,-999);

    for(size_t i=0; i<nbOfVerticesSide; i++){
        for(size_t j=0; j<nbOfVerticesSide; j++){
            glm::vec3 vertexPos = edgeLength * glm::vec3(i, 0, j) - offset;
            
            for(int i=0; i<3; i++){
                if(min[i] > vertexPos[i]) min[i] = vertexPos[i];
                if(max[i] < vertexPos[i]) max[i] = vertexPos[i];
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

    std::vector<float> vertex_buffer_data;
    
    for(int i=0; i<indexed_vertices.size(); i++){
        glm::vec3 vertex = indexed_vertices[i];
        glm::vec3 n = normal[i];

        vertex_buffer_data.push_back(vertex.x);
        vertex_buffer_data.push_back(vertex.y);
        vertex_buffer_data.push_back(vertex.z);
    
        vertex_buffer_data.push_back(tex_coords[i].x);
        vertex_buffer_data.push_back(tex_coords[i].y);

        vertex_buffer_data.push_back(n.x);
        vertex_buffer_data.push_back(n.y);
        vertex_buffer_data.push_back(n.z);

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
        {{-halfLength,  halfLength, -halfLength}, {edgeIncrement, 0, 0}, {0, 0, edgeIncrement}, {0, 1, 0}},
        // -Y (bottom)
        {{-halfLength, -halfLength,  halfLength}, {edgeIncrement, 0, 0}, {0, 0, -edgeIncrement}, {0, -1, 0}},
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

                if (inward) {
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

    // Create interleaved vertex buffer
    std::vector<float> vertexBuffer;
    vertexBuffer.reserve(vertices.size() * 8);
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        // Position
        vertexBuffer.push_back(vertices[i].x);
        vertexBuffer.push_back(vertices[i].y);
        vertexBuffer.push_back(vertices[i].z);
        
        // Texture coordinates
        vertexBuffer.push_back(texCoords[i].x);
        vertexBuffer.push_back(texCoords[i].y);
        
        // Normal
        vertexBuffer.push_back(normals[i].x);
        vertexBuffer.push_back(normals[i].y);
        vertexBuffer.push_back(normals[i].z);
    }

    result.indexCount = static_cast<unsigned int>(indices.size());
    result.init(vertexBuffer, indices);
    
    return result;
}





Drawable Render::loadMesh(char *filePath){
    Drawable res;

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType |
        aiProcess_FlipUVs); // si besoin

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return res;
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
    }


    res.indexCount = indices.size();

    std::vector<float> vertex_buffer_data;
    for (int i = 0; i < indexed_vertices.size(); ++i) {
        glm::vec3 v = indexed_vertices[i];
        glm::vec3 n = normal[i];
        glm::vec2 t = tex_coords[i];

        vertex_buffer_data.insert(vertex_buffer_data.end(), {v.x, v.y, v.z, t.x, t.y, n.x, n.y, n.z});
    }

    res.init(vertex_buffer_data, indices);

    return res;
}


void CustomSystem::update(float deltaTime){
    for(auto const& entity: mEntities){
        auto& behavior = ecs.GetComponent<CustomBehavior>(entity);

        behavior.update(deltaTime);
    }
}