#pragma once

#include <vector>
#include <map>
#include <memory>
#include <engine/include/ecs/implementations/components.hpp>

class Material;

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
    static std::vector<std::unique_ptr<Program>> programs;

    Program() = default;
    Program(const char *vertexPath, const char *fragmentPath);

    virtual void beforeRender();
    virtual void afterRender();
    
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
};

class Skybox: public Program{
    GLuint skyboxID;

    public:
    Skybox();
    
    void setSkybox(std::vector<std::string> faces);
    void beforeRender() override;
    void afterRender() override;
};


class PBR: public Program{
    private:
    GLuint albedoLocation, metallicLocation, roughnessLocation, aoLocation, camPosLocation, hasTextureLocation, texLocation;

    public:
    PBR();
    void updateGUI() override;
    void updateMaterial(Material &value);
};


