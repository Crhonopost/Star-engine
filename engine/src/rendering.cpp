#include <GLES3/gl3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <engine/include/stbi.h>
#include <engine/include/spatial.hpp>
#include <engine/include/rendering.hpp>
#include <common/shader.hpp>
#include <engine/include/rendering.hpp>

std::vector<GLuint> freeIds;

std::map<std::string, Texture> Texture::textures;
std::vector<std::unique_ptr<Program>> Program::programs;

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
    glActiveTexture(GL_TEXTURE0);
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
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

Program::Program(const char *vertexPath, const char *fragmentPath){
    programID = LoadShaders( vertexPath, fragmentPath );
    glUseProgram(programID);

    modelLocation = glGetUniformLocation(programID, "model");
    vLocation = glGetUniformLocation(programID, "v");
    pLocation = glGetUniformLocation(programID, "p");

    glm::mat4 p = Camera::getInstance().getP();
    updateProjectionMatrix(p);
}

void Program::clear(){
    for (auto& [key, tex] : Texture::textures) {
        glDeleteTextures(1, &tex.id);
    }
    glDeleteProgram(programID);
}

void Program::updateViewMatrix(glm::mat4 &v){
    glUniformMatrix4fv(vLocation, 1, GL_FALSE, &v[0][0]);
}
void Program::updateProjectionMatrix(glm::mat4 &p){
    glUniformMatrix4fv(pLocation, 1, GL_FALSE, &p[0][0]);
}
void Program::updateModelMatrix(glm::mat4 model){
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
}



void Program::initTexture(char *path, char *uniformName){
    Texture &texture = Texture::loadTexture(path);

    GLuint textureLocation = glGetUniformLocation(programID, uniformName);

    programTextures.emplace(textureLocation, &texture);
}


void Program::renderTextures(int &activationInt){
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


void Program::updateGUI(){}

Material::Material(): Program("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl"){
    albedoLocation = glGetUniformLocation(programID, "albedo");
    metallicLocation = glGetUniformLocation(programID, "metallic");
    roughnessLocation = glGetUniformLocation(programID, "roughness");
    aoLocation = glGetUniformLocation(programID, "ao");
    camPosLocation = glGetUniformLocation(programID, "camPos");
    hasTextureLocation = glGetUniformLocation(programID, "hasTexture");
    texLocation = glGetUniformLocation(programID,"tex");
}

void Material::updateGUI(){
    glUseProgram(programID);
    if(ImGui::SliderFloat("metallic", &metallic, 0.0f, 5.0f)){
        glUniform1f(metallicLocation, metallic);
    }

    if(ImGui::SliderFloat("roughness", &roughness, 0.0f, 5.0f)){
        glUniform1f(roughnessLocation, roughness);
    }

    if(ImGui::SliderFloat("ao", &ao, 0.0f, 5.0f)){
        glUniform1f(aoLocation, ao);
    }
}
