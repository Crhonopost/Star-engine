#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <engine/include/API/ResourceManagement/IResource.hpp>


struct Texture: public IResource {
    std::string path;
    GLuint id;
    bool visible;
    
    Texture(): id(0), visible(false){};
    
    bool loadTextureFromData(const unsigned char* data,
                                          size_t size,
                                          int width,
                                          int height,
                                          int channels,
                                          const std::string& key);
    bool load(const std::string &name) override;
    

    static std::shared_ptr<Texture> emptyTexture;

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
    std::unordered_map<GLuint, std::shared_ptr<Texture>> programTextures;
    
    public:
    GLuint programID;

    Program() = default;
    Program(const char *vertexPath, const char *fragmentPath);

    virtual void beforeRender();
    virtual void afterRender();
    
    void clear();
    void renderTextures();
    void initTexture(char *path, char *uniformName);
    virtual void updateGUI();

    void updateViewMatrix(const glm::mat4 &v);
    void updateProjectionMatrix(const glm::mat4 &p);
    void updateModelMatrix(const glm::mat4 &model);

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


