#include <GLES3/gl3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <engine/include/stbi.h>
#include <engine/include/spatial.hpp>
#include <engine/include/rendering.hpp>
#include <common/shader.hpp>
#include <engine/include/rendering.hpp>
#include <fstream>

std::map<std::string, Texture> Texture::textures;
std::vector<std::unique_ptr<Program>> Program::programs;
int Texture::activationInt = 0;

int Texture::getAvailableActivationInt(){
    return activationInt ++;
}

void Texture::resetActivationInt(){
    activationInt = 0;
}


Texture Texture::emptyTexture;

Texture& Texture::loadTextureFromMemory(const unsigned char* data,
                                         size_t size,
                                         int width,
                                         int height,
                                         int channels,
                                         const std::string& key)
{
    auto it = textures.find(key);
    if (it != textures.end())
        return it->second;

    Texture tex;
    tex.path = key.c_str();
    glGenTextures(1, &tex.id);
    int unit = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char* imageData = nullptr;
    int imgWidth = width;
    int imgHeight = height;
    int imgChannels = channels;

    if (size > 0 && height == 0) {
        // compressed
        imageData = stbi_load_from_memory(data, static_cast<int>(size), &imgWidth, &imgHeight, &imgChannels, 0);
        if (!imageData) {
            std::cerr << "Failed to load embedded texture from memory: " << key << std::endl;
        }
    } else {
        imageData = const_cast<unsigned char*>(data);
    }

    GLenum format = (imgChannels == 4 ? GL_RGBA : (imgChannels == 3 ? GL_RGB : GL_RED));
    glTexImage2D(GL_TEXTURE_2D, 0, format, imgWidth, imgHeight, 0, format, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (size > 0 && height == 0 && imageData)
        stbi_image_free(imageData);

    auto inserted = textures.emplace(key, tex);
    return inserted.first->second;
}

Texture& Texture::loadTexture(const char * path){
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

    glGenTextures(1, &texture.id);
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
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
    glBindTexture(GL_TEXTURE_2D, 0); // TODO: usefull ?

    return texture;
}

void Texture::activate(GLuint textureLocation){
    glBindTextureUnit(activationInt, id);
    glUniform1i(textureLocation, activationInt);
    activationInt ++;
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

void Program::destroyPrograms(){
    for(int i=0; i<programs.size(); i++){
        programs[i]->clear();
    }

    programs.clear();
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

void Program::beforeRender(){}
void Program::afterRender(){}

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

        texture->activate(textureLocation);
    }
}

void PBR::updateLightCount(int count){
    GLuint lightCountLocation = glGetUniformLocation(programID, "lightCount");
    glUniform1i(lightCountLocation, count);
}
void PBR::updateLightPosition(int lightIndex, glm::vec3 position){
    GLuint lightLocation = glGetUniformLocation(programID, ("lightPositions[" + std::to_string(lightIndex) + "]").c_str());
    glUniform3f(lightLocation, position[0], position[1], position[2]);
}
void PBR::updateLightColor(int lightIndex, glm::vec3 color){
    GLuint lightLocation = glGetUniformLocation(programID, ("lightColors[" + std::to_string(lightIndex) + "]").c_str());
    glUniform3f(lightLocation, color[0], color[1], color[2]);
}

void Skybox::beforeRender(){
    glDepthMask(GL_FALSE);

    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.textureID);
    GLuint loc = glGetUniformLocation(programID, "skybox");
    glUniform1i(loc, current);
}

void Skybox::afterRender(){
    glDepthMask(GL_TRUE);
}

Skybox::Skybox(Cubemap sky):Program("shaders/skybox/vertex.glsl", "shaders/skybox/fragment.glsl"), cubemap(sky){}

IrradianceShader::IrradianceShader():Program("shaders/skybox/vertex.glsl", "shaders/skybox/irradiance_convolution.glsl"){}

PrefilterShader::PrefilterShader():Program("shaders/skybox/vertex.glsl", "shaders/skybox/prefilter.glsl"){}

BrdfShader::BrdfShader():Program("shaders/skybox/BRDF_vs.glsl", "shaders/skybox/BRDF_fs.glsl"){}

CubemapProg::CubemapProg(): Program("shaders/cubemap/vertex.glsl", "shaders/cubemap/fragment.glsl"){}

void CubemapProg::beforeRender(){
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    GLuint loc = glGetUniformLocation(programID, "skybox");
    glUniform1i(loc, current);
}

Cubemap::Cubemap(int resolution){
    this->resolution = resolution;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    for (GLuint i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_RGB16F,                    // ou GL_RGBA selon ton besoin
            resolution,
            resolution,
            0,
            GL_RGB,                    // même format ici
            GL_FLOAT,
            nullptr                    // pas de données, juste allocation
        );
    }    

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

Cubemap::Cubemap(std::vector<std::string> paths){
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < 6; i++)
    {
        unsigned char *data = stbi_load(paths[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << paths[i] << std::endl;
            stbi_image_free(data);
        }
    }
    this->resolution = width;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Cubemap::clear(){
    glDeleteTextures(1, &textureID);
}

// void Cubemap::setFace(int idx, unsigned char *data){
//     glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx, 
//                  0, GL_RGB, resolution, resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, data
//     );
// }

void Program::updateGUI(){}

PBR::PBR(): Program("shaders/pbr/vertex_shader.glsl", "shaders/pbr/fragment_shader.glsl"){
    albedoLocation = glGetUniformLocation(programID, "albedoVal");
    metallicLocation = glGetUniformLocation(programID, "metallicVal");
    roughnessLocation = glGetUniformLocation(programID, "roughnessVal");
    aoLocation = glGetUniformLocation(programID, "aoVal");

    hasTextureLocation = glGetUniformLocation(programID, "hasTexture");

    albedoTexLocation = glGetUniformLocation(programID, "albedoMap");
    metallicTexLocation = glGetUniformLocation(programID, "metallicMap");
    aoTexLocation = glGetUniformLocation(programID, "aoMap");
    normalTexLocation = glGetUniformLocation(programID, "normalMap");
    roughnessTexLocation = glGetUniformLocation(programID, "roughnessMap");

    indensiteScaleLightLocation = glGetUniformLocation(programID,"indensiteScaleLight");

    hasAlbedoMapLocation = glGetUniformLocation(programID, "hasAlbedoMap");
    hasNormalMapLocation = glGetUniformLocation(programID, "hasNormalMap");
    hasMetallicMapLocation = glGetUniformLocation(programID, "hasMetallicMap");
    hasRoughnessMapLocation = glGetUniformLocation(programID, "hasRoughnessMap");
    hasAoMapLocation = glGetUniformLocation(programID, "hasAoMap");
}

void PBR::updateMaterial(Material &material){
    glUniform1f(metallicLocation, material.metallic);
    glUniform1f(roughnessLocation, material.roughness);
    glUniform1f(aoLocation, material.ao);
    glUniform3f(albedoLocation, material.albedo[0], material.albedo[1], material.albedo[2]);
    
    if (material.albedoTex->visible){
        material.albedoTex->activate(albedoTexLocation);
        glUniform1i(hasAlbedoMapLocation, 1);
    } else {
        glUniform1i(hasAlbedoMapLocation, 0);
    }
    
    if (material.metallicTex->visible){
        material.metallicTex->activate(metallicTexLocation);
        glUniform1i(hasMetallicMapLocation, 1);
    } else {
        glUniform1i(hasMetallicMapLocation, 0);
    }
    
    if (material.aoTex->visible){
        material.aoTex->activate(aoTexLocation);
        glUniform1i(hasAoMapLocation, 1);
    } else {
        glUniform1i(hasAoMapLocation, 0);
    }
    
    if (material.normalTex->visible){
        material.normalTex->activate(normalTexLocation);
        glUniform1i(hasNormalMapLocation, 1);
    } else {
        glUniform1i(hasNormalMapLocation, 0);
    }
    
    if (material.roughnessTex->visible){
        material.roughnessTex->activate(roughnessTexLocation);
        glUniform1i(hasRoughnessMapLocation, 1);
    } else {
        glUniform1i(hasRoughnessMapLocation, 0);
    }
}

void PBR::updateGUI(){}


void save_PPM_file(int width, int height, const std::string& filename) {
    std::ofstream output_image(filename);
    unsigned char* pixels = new unsigned char[width * height * 3];

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    output_image << "P3\n";
    output_image << width << " " << height << "\n";
    output_image << "255\n";

    for (int j = height - 1; j >= 0; --j) { // image OpenGL = bottom to top
        for (int i = 0; i < width; ++i) {
            int idx = (j * width + i) * 3;
            output_image << (int)pixels[idx] << " "
                         << (int)pixels[idx + 1] << " "
                         << (int)pixels[idx + 2] << " ";
        }
        output_image << "\n";
    }

    delete[] pixels;
    output_image.close();
}

