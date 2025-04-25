#pragma once

#include <vector>
#include <map>
#include <memory>

class Drawable;

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
    GLuint skyboxID;

    public:
    Skybox();
    
    void setSkybox(std::vector<std::string> faces);
    void beforeRender() override;
    void afterRender() override;
    GLuint getSkyboxID() const { return skyboxID; }
};

class IrradianceShader:public Program{
    public:
    IrradianceShader();
    
};

class Material: public Program{
    private:
    GLuint albedoLocation, metallicLocation, roughnessLocation, aoLocation, camPosLocation, 
        hasTextureLocation, texLocation,indensiteScaleLightLocation;
    glm::vec3 albedo, camPos;
    float metallic = 0.5f;
    float roughness = 0.5f;
    float ao = 1.0f;
    float indensiteScaleLight = 100.f;
    bool hasTexture;

    std::vector<std::string> materialOptions = {
        "../assets/images/PBR/gold/",
        "../assets/images/PBR/oldMetal/",
        "../assets/images/PBR/rock/",
        "../assets/images/PBR/woods/"
    };
    std::vector<std::string> materialFormats = {
        ".png",
        ".png",
        ".jpg",
        ".jpg"
    };
    int currentMaterialIndex = 1;

    public:
    Material();
    void updateGUI() override;
    void loadCurrentMaterial();
};


GLuint generateIrradianceMap(GLuint envCubemap, Program* irradianceProgram, Drawable* cubeDrawable);