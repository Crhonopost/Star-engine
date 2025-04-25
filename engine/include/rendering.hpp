#pragma once

#include <vector>
#include <map>
#include <memory>

#include <engine/include/ecs/implementations/components.hpp>
class Drawable;

class Material;

struct Texture {
    char *path;
    GLuint id;

    Texture(): id(0){};

    static Texture& loadTexture(char * path);
    static std::map<std::string, Texture> textures;
};


struct Cubemap {
    GLuint textureID;
    int resolution;
    Cubemap(int resolution);
    // void setTop(unsigned char *data);
};

class Program {
    private:
    GLuint modelLocation, vLocation, pLocation;

    protected:
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

    void use() {
        glUseProgram(programID);
    }
};

class Skybox: public Program{
    public:
    GLuint skyboxID;
    Skybox();
    
    void setSkybox(std::vector<std::string> faces);
    void beforeRender() override;
    void afterRender() override;
};

class IrradianceShader:public Program{
    public:
    IrradianceShader();
    
};


class CubemapProg: public Program {
    public:
    GLuint textureID;

    CubemapProg();

    void beforeRender() override;
};

class CubemapProg: public Program {
    public:
    GLuint textureID;

    CubemapProg();

    void beforeRender() override;
};


class PBR: public Program{
    private:
    GLuint albedoLocation, metallicLocation, roughnessLocation, aoLocation, hasTextureLocation, indensiteScaleLightLocation;
    GLuint albedoTexLocation, metallicTexLocation, roughnessTexLocation, aoTexLocation;

    public:
    PBR();
    void updateGUI() override;
    void updateMaterial(Material &value);

    void updateLightCount(int count);
    void updateLightPosition(int lightIndex, glm::vec3 position);
    void updateLightColor(int lightIndex, glm::vec3 color);
};

void save_PPM_file(int width, int height, const std::string& filename);

GLuint generateIrradianceMap(GLuint envCubemap, Program* irradianceProgram, Drawable* cubeDrawable);