#include <GLES3/gl3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <engine/include/stbi.h>
#include <engine/include/spatial.hpp>
#include <engine/include/program.hpp>
#include <common/shader.hpp>

TextureData Program::loadTexture(char * path, char *uniformName){
    TextureData texture;
    texture.path = path;
    texture.uniformName = uniformName;
    texture.activationInt = textures.size();

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

    GLenum format;// = textureChannels == 1 ? GL_RED : GL_RGB;
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
    }

    glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, textureData);
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

Program::Program(char *vertexPath, char *fragmentPath){
    programID = LoadShaders( vertexPath, fragmentPath );
    glUseProgram(programID);

    modelLocation = glGetUniformLocation(programID, "model");
    vpLocation = glGetUniformLocation(programID, "vp");
}

void Program::clear(){
    for(auto meshPtr: meshes){
        spatial::MeshInstance &mesh = *meshPtr;
    
        glDeleteBuffers(1, &mesh.meshData.VBO);
        glDeleteBuffers(1, &mesh.meshData.EBO);
        glDeleteVertexArrays(1, &mesh.meshData.VAO);
    }

    glDeleteProgram(programID);
}

void Program::renderTextures(){
    for(TextureData tex : textures){
        tex.activate();
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glUniform1i(tex.uniformLocation, tex.activationInt);
    }
}

void Program::render(){
    for(auto meshPtr: meshes){
        spatial::MeshInstance &mesh = *meshPtr;
        updateModelMatrix(mesh.transform.getModelMatrix());

        mesh.texture.activate();
        glUniform1i(mesh.texture.uniformLocation, mesh.texture.activationInt);
        glBindTexture(GL_TEXTURE_2D, mesh.texture.id);
    
        glBindVertexArray(mesh.meshData.VAO);
    
        // Draw the triangles !
        glDrawElements(
                    GL_TRIANGLES,      // mode
                    mesh.getNumberOfIndices(),    // count
                    GL_UNSIGNED_SHORT,   // type
                    (void*)0           // element array buffer offset
                    );
    }
}

void Program::updateViewProjectionMatrix(glm::mat4 &vp){
    glUniformMatrix4fv(vpLocation, 1, GL_FALSE, &vp[0][0]);
}
void Program::updateModelMatrix(glm::mat4 model){
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
}