#include <GLES3/gl3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <engine/include/stbi.h>
#include <engine/include/spatial.hpp>
#include <engine/include/rendering.hpp>
#include <common/shader.hpp>
#include <engine/include/rendering.hpp>

int activationInt = 0;
std::vector<GLuint> freeIds;

std::map<std::string, Texture> Texture::textures;
std::vector<Program> Program::programs;

void Texture::generateTextures(int count) {
    freeIds.resize(count);
    glGenTextures(count, freeIds.data());

    for (int i = 0; i < count; i++) {
        std::cout << "Generated Texture ID: " << freeIds[i] << std::endl;
    }
}

Texture& Texture::loadTexture(char * path){
    std::string key(path);
    auto it = textures.find(key);
    if (it != textures.end()) {
        return it->second;
    }

    
    Texture tex;
    textures.emplace(key, tex);


    it = textures.find(key);
    Texture &texture = it->second;
    texture.path = path;

    // glGenTextures(1, &texture.id);
    texture.id = freeIds[freeIds.size()-1];
    freeIds.pop_back();
    glActiveTexture(GL_TEXTURE0 + activationInt);
    glBindTexture(GL_TEXTURE_2D, texture.id);
 

    GLint checkBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &checkBinding);
    if (checkBinding != texture.id) {
        std::cerr << "Warning: Texture not bound correctly! Expected " 
                << texture.id << ", got " << checkBinding << "\n";
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char *textureData;
    int textureWidth, textureHeight, textureChannels;
    textureData = stbi_load(path, &textureWidth, &textureHeight, &textureChannels, 0);

    if(!textureData){
        std::cerr << "Failed to load texture\n";
    }

    GLenum format = GL_RGB;
    switch(textureChannels){
        case 1:
            format = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            std::cerr << "Warning: Unsupported texture format, defaulting to GL_RGB\n";
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, textureData);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(textureData);

    // glUseProgram(programID);
    // texture.uniformLocation = glGetUniformLocation(programID, texture.uniformName);        
    // if(texture.uniformLocation == -1){
    //     std::cerr << "Invalid uniform location, name: " << texture.uniformName << "\n";
    // }

    // glUniform1i(texture.uniformLocation, activationInt);
    // glBindTexture(GL_TEXTURE_2D, texture.id);

    textures.emplace(texture.path, texture);
    activationInt ++;
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

Program::Program(char *vertexPath, char *fragmentPath){
    programID = LoadShaders( vertexPath, fragmentPath );
    glUseProgram(programID);

    modelLocation = glGetUniformLocation(programID, "model");
    vpLocation = glGetUniformLocation(programID, "vp");
}

void Program::clear(){
    for (auto& [key, tex] : Texture::textures) {
        glDeleteTextures(1, &tex.id);
    }
    glDeleteProgram(programID);
}

void Program::updateViewProjectionMatrix(glm::mat4 &vp){
    glUniformMatrix4fv(vpLocation, 1, GL_FALSE, &vp[0][0]);
}
void Program::updateModelMatrix(glm::mat4 model){
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
}



void Program::initTexture(char *path, char *uniformName){
    Texture &texture = Texture::loadTexture(path);

    GLuint textureLocation = glGetUniformLocation(programID, uniformName);

    programTextures.emplace(textureLocation, &texture);
}


void Program::renderTextures(){
    glUseProgram(programID);
    for (auto& [textureLocation, texture] : programTextures) {
        if (texture->id == 0){
            std::cerr << "ID de texture invalide\n";
            continue;
        }

        glBindTextureUnit(activationInt, texture->id);
        glUniform1i(textureLocation, activationInt);

        activationInt++;
    }
}

