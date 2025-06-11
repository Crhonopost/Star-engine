#include <engine/include/spatial.hpp> // ?
#include <engine/include/rendering/rendering.hpp>
#include <common/shader.hpp>

Program::Program(const char *vertexPath, const char *fragmentPath){
    programID = LoadShaders( vertexPath, fragmentPath );
    glUseProgram(programID);

    modelLocation = glGetUniformLocation(programID, "model");
    vLocation = glGetUniformLocation(programID, "v");
    pLocation = glGetUniformLocation(programID, "p");

    glm::mat4 p = Camera::getInstance().getP();
    updateProjectionMatrix(p);
    glm::mat4 camProj = Camera::getInstance().getP();
    updateProjectionMatrix(camProj);
}

void Program::clear(){
    for (auto& [key, tex] : Texture::textures) {
        glDeleteTextures(1, &tex.id);
    }
    glDeleteProgram(programID);
}

void Program::destroyPrograms(){
    // for(int i=0; i<programs.size(); i++){
    //     programs[i]->clear();
    // }

    // programs.clear();
}

void Program::updateViewMatrix(const glm::mat4 &v){
    glUniformMatrix4fv(vLocation, 1, GL_FALSE, &v[0][0]);
}
void Program::updateProjectionMatrix(const glm::mat4 &p){
    glUniformMatrix4fv(pLocation, 1, GL_FALSE, &p[0][0]);
}
void Program::updateModelMatrix(const glm::mat4 &model){
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

void ProgSkybox::beforeRender(){
    glUseProgram(programID);
    glDepthMask(GL_FALSE);

    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.textureID);
    GLuint loc = glGetUniformLocation(programID, "skybox");
    glUniform1i(loc, current);
}

void ProgSkybox::afterRender(){
    glDepthMask(GL_TRUE);
}

ProgSkybox::ProgSkybox(Cubemap sky):Program("shaders/skybox/vertex.glsl", "shaders/skybox/fragment.glsl"), cubemap(sky){}

ProgIrradiance::ProgIrradiance():Program("shaders/skybox/vertex.glsl", "shaders/skybox/irradiance_convolution.glsl"){}

ProgPrefilter::ProgPrefilter():Program("shaders/skybox/vertex.glsl", "shaders/skybox/prefilter.glsl"){}

ProgBRDF::ProgBRDF():Program("shaders/skybox/BRDF_vs.glsl", "shaders/skybox/BRDF_fs.glsl"){}

ProgCubemap::ProgCubemap(): Program("shaders/cubemap/vertex.glsl", "shaders/cubemap/fragment.glsl"){}







void ProgCubemap::beforeRender(){
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    GLuint loc = glGetUniformLocation(programID, "skybox");
    glUniform1i(loc, current);
}

void Program::updateGUI(){}


void ProgPBR::updateLightCount(int count){
    GLuint lightCountLocation = glGetUniformLocation(programID, "lightCount");
    glUniform1i(lightCountLocation, count);
}

void ProgPBR::updateLightPosition(int lightIndex, glm::vec3 position){
    GLuint lightLocation = glGetUniformLocation(programID, ("lightPositions[" + std::to_string(lightIndex) + "]").c_str());
    glUniform3f(lightLocation, position[0], position[1], position[2]);
}

void ProgPBR::updateLightColor(int lightIndex, glm::vec3 color){
    GLuint lightLocation = glGetUniformLocation(programID, ("lightColors[" + std::to_string(lightIndex) + "]").c_str());
    glUniform3f(lightLocation, color[0], color[1], color[2]);
}

ProgPBR::ProgPBR(): Program("shaders/pbr/vertex_shader.glsl", "shaders/pbr/fragment_shader.glsl"){
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

void ProgPBR::updateMaterial(Material &material){
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

void ProgPBR::updateGUI(){}