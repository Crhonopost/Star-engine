#pragma once

#include <vector>
#include <map>
#include <memory>

struct Texture {
    char *path;
    GLuint id;

    Texture(): id(0){};

    static Texture& loadTexture(char * path);
    static void generateTextures(int count);

    static std::map<std::string, Texture> textures;
};


class Program {
    private:
    GLuint modelLocation, vLocation, pLocation;

    // Setup à l'initialisation uniquement
    std::map<GLuint, Texture*> programTextures;
    
    public:
    GLuint programID;

    Program() = default;
    Program(const char *vertexPath, const char *fragmentPath);
    
    void clear();
    void renderTextures(int &activationInt);
    void initTexture(char *path, char *uniformName);
    virtual void updateGUI();

    void updateViewMatrix(glm::mat4 &v);
    void updateProjectionMatrix(glm::mat4 &p);
    void updateModelMatrix(glm::mat4 model);

    void updateLightCount(int count);
    void updateLightPosition(int lightIndex, glm::vec3 position);
    void updateLightColor(int lightIndex, glm::vec3 color);

    static std::vector<std::unique_ptr<Program>> programs;
};


class Material: public Program{
    private:
    GLuint albedoLocation, metallicLocation, roughnessLocation, aoLocation, camPosLocation, hasTextureLocation, texLocation;
    glm::vec3 albedo, camPos;
    float metallic = 0.5f;
    float roughness = 0.5f;
    float ao = 1.0f;

    public:
    Material();
    void updateGUI() override;
};


