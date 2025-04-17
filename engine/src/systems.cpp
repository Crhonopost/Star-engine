#include <GL/glew.h>
#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/rendering.hpp>
#include <engine/include/camera.hpp>
#include <iostream>


void Render::update() {
    
    glm::mat4 V = Camera::getInstance().getV();
    int activationInt = 0;
    
    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<Drawable>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        
        float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
        auto& program = *drawable.program;
        glUseProgram(program.programID);
        
        glm::mat4 model = transform.getModelMatrix();

        program.renderTextures(activationInt);

        program.updateModelMatrix(model);
        program.updateViewMatrix(V);

        drawable.draw(distanceToCam);
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


void CustomSystem::update(float deltaTime){
    for(auto const& entity: mEntities){
        auto& behavior = ecs.GetComponent<CustomBehavior>(entity);

        behavior.update(deltaTime);
    }
}