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

    glGenTextures(1, &texture.id);
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

void Texture::activate(GLuint textureLocation, int activationInt){
    glBindTextureUnit(activationInt, id);
    glUniform1i(textureLocation, activationInt);
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);
    GLint loc = glGetUniformLocation(programID, "skybox");
    glUniform1i(loc, 0);
}

void Skybox::afterRender(){
    glDepthMask(GL_TRUE);
}

Skybox::Skybox():Program("shaders/skybox/vertex.glsl", "shaders/skybox/fragment.glsl")
{
    setSkybox({
        "../assets/images/cubemaps/cloudy/bluecloud_rt.jpg",
        "../assets/images/cubemaps/cloudy/bluecloud_lf.jpg",
        "../assets/images/cubemaps/cloudy/bluecloud_up.jpg",
        "../assets/images/cubemaps/cloudy/bluecloud_dn.jpg",
        "../assets/images/cubemaps/cloudy/bluecloud_bk.jpg",
        "../assets/images/cubemaps/cloudy/bluecloud_ft.jpg"});
} 
IrradianceShader::IrradianceShader():Program("shaders/skybox/vertex.glsl", "shaders/skybox/irradiance_convolution.glsl"){}

PrefilterShader::PrefilterShader():Program("shaders/skybox/vertex.glsl", "shaders/skybox/prefilter.glsl"){}

BrdfShader::BrdfShader():Program("shaders/skybox/BRDF_vs.glsl", "shaders/skybox/BRDF_fs.glsl"){}

void Skybox::setSkybox(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    skyboxID = textureID;
}

CubemapProg::CubemapProg(): Program("shaders/cubemap/vertex.glsl", "shaders/cubemap/fragment.glsl"){}

void CubemapProg::beforeRender(){
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    GLuint loc = glGetUniformLocation(programID, "skybox");
    glUniform1i(loc, 0);
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
}

void PBR::updateMaterial(Material &material,int &activationInt){
    glUniform1f(metallicLocation, material.metallic);
    glUniform1f(roughnessLocation, material.roughness);
    glUniform1f(aoLocation, material.ao);
    glUniform3f(albedoLocation, material.albedo[0], material.albedo[1], material.albedo[2]);
    glUniform1i(hasTextureLocation, material.hasTexture);

    if(material.hasTexture){
        material.albedoTex->activate(albedoTexLocation, activationInt++);
        material.metallicTex->activate(metallicTexLocation, activationInt++);
        material.aoTex->activate(aoTexLocation, activationInt++);
        material.normalTex->activate(normalTexLocation, activationInt++);
        material.roughnessTex->activate(roughnessTexLocation, activationInt++);
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

