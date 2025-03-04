#include <TP1/include/texture.hpp>
#include <GLES3/gl3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <TP1/include/stbi.h>

using namespace Texture;

std::vector<tex> textures;

tex TextureManager::loadTexture(char * path, char *uniformName, GLuint programID){
    tex texture;
    texture.path = path;
    texture.uniformName = uniformName;
    texture.activationInt = GL_TEXTURE0 + textures.size();

    glGenTextures(1, &texture.id);
    texture.activate();
    glBindTexture(GL_TEXTURE_2D, texture.id);

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

    GLenum format = textureChannels == 1 ? GL_RED : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, textureData);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(textureData);


    glUseProgram(programID);
    texture.uniformLocation = glGetUniformLocation(programID, texture.uniformName);        
    if(texture.uniformLocation == -1){
        std::cerr << "Invalid uniform location, name: " << texture.uniformName << "\n";
    }
    
    glUniform1i(texture.uniformLocation, textures.size());        
    glBindTexture(GL_TEXTURE_2D, texture.id);

    textures.push_back(texture);

    return texture;
}

void TextureManager::render(GLuint programID){
    for(tex tex : textures){
        tex.activate();
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glUniform1i(tex.uniformLocation, tex.activationInt);
    }
}