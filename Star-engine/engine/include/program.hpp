#pragma once

#include <vector>

#include <engine/include/spatial.hpp>

class Program {
    GLuint modelLocation, vpLocation;
    std::vector<TextureData> textures;
    
    public:
    std::vector<spatial::MeshInstance*> meshes;
    GLuint programID;

    Program(char *vertexPath, char *fragmentPath);
    void clear();

    TextureData loadTexture(char * path, char *uniformName);

    void renderTextures();

    void render();
    void updateViewProjectionMatrix(glm::mat4 &vp);
    void updateModelMatrix(glm::mat4 model);
};