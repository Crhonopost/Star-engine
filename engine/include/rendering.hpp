#pragma once

#include <vector>
#include <map>

struct Texture {
    char *path;
    GLuint id;

    Texture(): id(0){};

    static Texture& loadTexture(char * path);
    static void generateTextures(int count);
};


class Program {
    private:
    GLuint modelLocation, vpLocation;

    // Setup à l'initialisation uniquement
    std::map<GLuint, Texture*> programTextures;
    
    public:
    GLuint programID;

    Program() = default;
    Program(char *vertexPath, char *fragmentPath);
    
    void clear();
    void renderTextures();
    void initTexture(char *path, char *uniformName);
    void updateViewProjectionMatrix(glm::mat4 &vp);
    void updateModelMatrix(glm::mat4 model);
};


// class Material: public Program{

// };

extern std::vector<Program> programs;
extern std::map<std::string, Texture> textures;


