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



void Render::update() {
    
    glm::mat4 V = Camera::getInstance().getV();
    int activationInt = 0;
    
    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<Drawable>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        
        float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
        auto& program = *drawable.program;
        glUseProgram(program.programID);
        program.beforeRender();
        
        glm::mat4 model = transform.getModelMatrix();

        program.renderTextures(activationInt);

        program.updateViewMatrix(V);
        program.updateModelMatrix(model);

        drawable.draw(distanceToCam);
        program.afterRender();
    }

}

void LightRender::update(){
    //TODO: update as a batch https://gamedev.stackexchange.com/questions/179539/how-to-set-the-value-of-each-index-in-a-uniform-array
    int associatedLight = 0;
    for (const auto& entity : mEntities) {
        auto& light = ecs.GetComponent<Light>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);

        for(auto &prog : Program::programs){
            glUseProgram(prog->programID);
            prog->updateLightPosition(associatedLight, transform.getLocalPosition());
            prog->updateLightColor(associatedLight, light.color);
        }
        associatedLight ++;
    }
    
    for(auto &prog : Program::programs){
        glUseProgram(prog->programID);
        prog->updateLightCount(associatedLight);
    }
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

Drawable Render::generateInwardCube(float sideLength, int nbOfVerticesSide) {
    Drawable res;

    std::vector<unsigned short> indices;
    std::vector<glm::vec3> indexed_vertices;
    std::vector<glm::vec2> tex_coords;
    std::vector<glm::vec3> normal;

    float edge = sideLength / (nbOfVerticesSide - 1);
    float half = sideLength / 2.0f;

    // 6 faces
    struct Face {
        glm::vec3 origin;
        glm::vec3 u_dir;
        glm::vec3 v_dir;
        glm::vec3 normal;
    };

    std::vector<Face> faces = {
        // Face +Y (haut)
        {{-half, half, -half}, {edge, 0, 0}, {0, 0, edge}, {0, -1, 0}},
        // Face -Y (bas)
        {{-half, -half, half}, {edge, 0, 0}, {0, 0, -edge}, {0, 1, 0}},
        // Face +X
        {{half, -half, -half}, {0, edge, 0}, {0, 0, edge}, {-1, 0, 0}},
        // Face -X
        {{-half, -half, half}, {0, edge, 0}, {0, 0, -edge}, {1, 0, 0}},
        // Face +Z
        {{-half, -half, half}, {edge, 0, 0}, {0, edge, 0}, {0, 0, -1}},
        // Face -Z
        {{half, -half, -half}, {-edge, 0, 0}, {0, edge, 0}, {0, 0, 1}},
    };

    int vertexOffset = 0;

    for (auto& face : faces) {
        for (int i = 0; i < nbOfVerticesSide; ++i) {
            for (int j = 0; j < nbOfVerticesSide; ++j) {
                glm::vec3 pos = face.origin + float(i) * face.u_dir + float(j) * face.v_dir;
                indexed_vertices.push_back(pos);
                normal.push_back(face.normal);
                tex_coords.push_back(glm::vec2((float)i / (nbOfVerticesSide - 1), (float)j / (nbOfVerticesSide - 1)));
            }
        }

        for (int i = 0; i < nbOfVerticesSide - 1; ++i) {
            for (int j = 0; j < nbOfVerticesSide - 1; ++j) {
                int a = vertexOffset + i * nbOfVerticesSide + j;
                int b = a + 1;
                int c = a + nbOfVerticesSide;
                int d = c + 1;

                // Inversé pour normales vers l'intérieur
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(b);

                indices.push_back(b);
                indices.push_back(c);
                indices.push_back(d);
            }
        }

        vertexOffset += nbOfVerticesSide * nbOfVerticesSide;
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