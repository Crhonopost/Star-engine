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
    
    void activate(GLuint textureLocation);
    static int getAvailableActivationInt();
    static void resetActivationInt();

    private:
    static int activationInt;
};


struct Cubemap {
    GLuint textureID;
    int resolution;
    Cubemap(int resolution);
    Cubemap(std::vector<std::string> paths);
    void clear();
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
    void renderTextures();
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
    void setFloat(char* uniformName,float valeur){
        glUniform1f(glGetUniformLocation(programID,uniformName),valeur);
    }
    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value); 
    }
    static void destroyPrograms();
};

class Skybox: public Program{
    public:
    GLuint octaProjLoc;
    Cubemap cubemap;
    Skybox(Cubemap sky);
    
    void beforeRender() override;
    void afterRender() override;
    void setProjectionOcta(bool state);
};

class IrradianceShader:public Program{
    public:
    IrradianceShader();
    
};
class PrefilterShader:public Program{
    public:
    PrefilterShader();
};

class BrdfShader:public Program{
    public:
    BrdfShader();
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
    GLuint albedoTexLocation, metallicTexLocation, roughnessTexLocation, aoTexLocation, normalTexLocation;
    

    public:
    PBR();
    void updateGUI() override;
    void updateMaterial(Material &value);

    void updateLightCount(int count);
    void updateLightPosition(int lightIndex, glm::vec3 position);
    void updateLightColor(int lightIndex, glm::vec3 color);
    
};

class ProbeProg: public Program{
    public:
    GLuint probeCountsLoc, probeStartPositionLoc, probeStepLoc, lowResolutionDownsampleFactorLoc;
    ProbeProg();
};

void save_PPM_file(int width, int height, const std::string& filename);