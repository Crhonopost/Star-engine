#pragma once

#include <vector>

#include <engine/include/ecs/implementations/components.hpp>

class Program {
    GLuint modelLocation, vpLocation;
    std::vector<Texture> textures;
    
    public:
    static std::vector<Program> programs;
    GLuint programID;

    Program(char *vertexPath, char *fragmentPath);
    void clear();

    Texture loadTexture(char * path, char *uniformName);

    void renderTextures();
    void updateViewProjectionMatrix(glm::mat4 &vp);
    void updateModelMatrix(glm::mat4 model);
    static void generateTextures(int count);
};
