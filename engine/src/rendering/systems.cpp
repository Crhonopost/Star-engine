#include <GLES3/gl3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <engine/include/stbi.h>
#include <engine/include/spatial.hpp>
#include <engine/include/rendering/rendering.hpp>
#include <engine/include/ecs/implementations/systems.hpp>
#include <common/shader.hpp>
#include <fstream>




int Texture::activationInt = 0;

int Texture::getAvailableActivationInt(){
    return activationInt ++;
}

void Texture::resetActivationInt(){
    activationInt = 0;
}


std::shared_ptr<Texture> Texture::emptyTexture = std::make_shared<Texture>();

bool Texture::loadTextureFromData(const unsigned char* data,
                                         size_t size,
                                         int width,
                                         int height,
                                         int channels,
                                         const std::string& key)
{
    path = key;
    glGenTextures(1, &id);
    int unit = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);

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

    return true;
}

bool Texture::load(const std::string &name) {
    path = name;

    glGenTextures(1, &id);
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_2D, id);
 

    GLint checkBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &checkBinding);
    if (checkBinding != id) {
        std::cerr << "Warning: Texture not bound correctly! Expected " 
                << id << ", got " << checkBinding << "\n";
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char *textureData;
    int textureWidth, textureHeight, textureChannels;
    textureData = stbi_load(path.c_str(), &textureWidth, &textureHeight, &textureChannels, 0);

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

    glBindTexture(GL_TEXTURE_2D, 0); // TODO: usefull ?

    return true;
}

void Texture::activate(GLuint textureLocation){
    glBindTextureUnit(activationInt, id);
    glUniform1i(textureLocation, activationInt);
    activationInt ++;
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


void renderQuad();

// void Render::update(glm::mat4 &view, bool isCubemapRender) {
        
//     for (const auto& entity : mEntities) {
//         auto& drawable = ecs.GetComponent<Drawable>(entity);
//         if (isCubemapRender && drawable.hideOnCubemapRender) {
//             continue;
//         }
//         auto& transform = ecs.GetComponent<Transform>(entity);
//         auto& program = *ecs.GetComponent<CustomProgram>(entity).programPtr;
        
//         float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
//         glUseProgram(program.programID);
//         program.beforeRender();
        
//         glm::mat4 model = transform.getModelMatrix();

//         program.renderTextures();

//         program.updateViewMatrix(view);
//         program.updateModelMatrix(model);
//         glm::mat4 camProj = Camera::getInstance().getP();
//         program.updateProjectionMatrix(camProj);

//         drawable.draw(distanceToCam);
//         program.afterRender();
//     }
// }

ProgPBR* SystemPBR::pbrProgPtr = nullptr;

void SystemPBR::initPBR(){
    if (pbrProgPtr == nullptr) {
        pbrProgPtr = new ProgPBR();
    }
}

void SystemPBR::setupMaps(){
    int current = Texture::getAvailableActivationInt();

    ProgPBR &pbrProg = *pbrProgPtr;
    glUseProgram(pbrProg.programID);

    GLuint camLoc = glGetUniformLocation(pbrProg.programID, "camPos");
    glm::vec3 C = Camera::getInstance().getPosition();
    glUniform3f(camLoc, C.x, C.y, C.z);

    ////////// irradiance map
    GLuint irrLoc = glGetUniformLocation(pbrProg.programID, "irradianceMap");
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMapID);
    glUniform1i(irrLoc, current);

    ////////// prefilter map
    GLuint prefiLoc = glGetUniformLocation(pbrProg.programID, "prefilterMap");
    current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilterMapID);
    glUniform1i(prefiLoc, current);
    ////////// brdf lut map
    GLuint brdfLoc = glGetUniformLocation(pbrProg.programID, "brdfLUTMap");
    current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_2D, mBrdfLUTID);
    glUniform1i(brdfLoc, current);
}

void SystemPBR::updateProjectionMatrix(glm::mat4 &proj) {
    ProgPBR &pbrProg = *pbrProgPtr;
    pbrProg.updateProjectionMatrix(proj);
}

void SystemPBR::update(glm::mat4 &view, bool isCubemapRender) {
    setupMaps();

    ProgPBR &pbrProg = *pbrProgPtr;
    pbrProg.beforeRender();
    pbrProg.updateViewMatrix(view);
    glm::mat4 camProj = Camera::getInstance().getP();
    pbrProg.updateProjectionMatrix(camProj);

    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<Drawable>(entity);
        if (isCubemapRender && drawable.hideOnCubemapRender) {
            continue;
        }
        auto& transform = ecs.GetComponent<Transform>(entity);
        auto& material = ecs.GetComponent<Material>(entity);

        pbrProg.updateMaterial(material);
        
        float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
        glm::mat4 model = transform.getModelMatrix();

        pbrProg.renderTextures();
        pbrProg.updateModelMatrix(model);

        drawable.draw(distanceToCam);
    }
    pbrProg.afterRender();
}


void SystemAnimatedPBR::update(glm::mat4 &view, float deltaTime){
    setupMaps();

    ProgPBR &pbrProg = *pbrProgPtr;
    pbrProg.beforeRender();
    pbrProg.updateViewMatrix(view);
    glm::mat4 camProj = Camera::getInstance().getP();
    pbrProg.updateProjectionMatrix(camProj);

    for (const auto& entity : mEntities) {
        auto& drawable = ecs.GetComponent<AnimatedDrawable>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        auto& material = ecs.GetComponent<Material>(entity);
        
        pbrProg.updateMaterial(material);
        
        float distanceToCam = glm::length(Camera::getInstance().camera_position - transform.getLocalPosition());
        
        glm::mat4 model = transform.getModelMatrix();
        
        pbrProg.renderTextures();
        pbrProg.updateModelMatrix(model);
        
        if(drawable.playing){
            drawable.animation.addDeltaTime(deltaTime);
        }
        std::vector<glm::mat4> inMatrices, outMatrices;
        
        drawable.animation.getPose(drawable.bones, inMatrices);
        
        CalculateAnimationPose(drawable.bones, inMatrices, outMatrices);
        for (int i = 0; i < outMatrices.size(); i++) {
            std::string uniformName = "bones[" + std::to_string(i) + "]";
            GLuint mLoc = glGetUniformLocation(pbrProg.programID, uniformName.c_str());
            glUniformMatrix4fv(mLoc, 1, GL_FALSE, &outMatrices[i][0][0]);
        }

        drawable.draw(distanceToCam);
    }
    pbrProg.afterRender();
}



void SystemLight::update(){
    //TODO: update as a batch https://gamedev.stackexchange.com/questions/179539/how-to-set-the-value-of-each-index-in-a-uniform-array
    int associatedLight = 0;
    glUseProgram(SystemPBR::pbrProgPtr->programID);
    for (const auto& entity : mEntities) {
        auto& light = ecs.GetComponent<Light>(entity);
        auto& transform = ecs.GetComponent<Transform>(entity);
        
        SystemPBR::pbrProgPtr->updateLightPosition(associatedLight, transform.getLocalPosition());
        SystemPBR::pbrProgPtr->updateLightColor(associatedLight, light.color);

        associatedLight ++;
    }
    
    SystemPBR::pbrProgPtr->updateLightCount(associatedLight);
}


CubemapRender::CubemapRender(int res): cubemap(res) {
    cubeMesh.mesh = MeshHelper::generateCube(10, 2, true);

    orientations[0] = {1,0,0};
    orientations[1] = {-1,0,0};
    ups[0] = ups[1] = {0,-1,0};
    
    orientations[2] = {0,1,0};
    orientations[3] = {0,-1,0};
    ups[2] = {0,0,1};
    ups[3] = {0,0,-1};
    
    orientations[4] = {0,0,1};
    orientations[5] = {0,0,-1};
    ups[4] = ups[5] = {0,-1,0};

    projection = glm::perspective(glm::radians(90.0f), 1.f, 0.1f, 100.f);
    
}


void CubemapRender::applyPrefilter(Program* prefilterProg, Cubemap prefilterMap) {
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap.textureID);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    GLuint fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.textureID);

    prefilterProg->use();
    GLuint skyLoc = glGetUniformLocation(prefilterProg->programID, "environmentMap");
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.textureID);
    glUniform1i(skyLoc, current);
    
    prefilterProg->beforeRender();
    prefilterProg->updateProjectionMatrix(projection);
    prefilterProg->updateModelMatrix(glm::mat4(1.0f));

    const unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipSize = static_cast<unsigned int>(prefilterMap.resolution * std::pow(0.5f, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);

        float roughness = float(mip) / float(maxMipLevels - 1);
        prefilterProg->use();
        prefilterProg->setFloat("roughness", roughness);

        
        for (int face = 0; face < 6; ++face) {
            
            glm::mat4 view = glm::lookAt(glm::vec3(0.0f), orientations[face], ups[face]);
            prefilterProg->updateViewMatrix(view);

            glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                prefilterMap.textureID,
                mip
            );
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cubeMesh.draw(-1);
        }
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap.textureID);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);


    prefilterProg->afterRender();
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}


GLuint CubemapRender::TwoDLUT(Program* brdfProg) {
    GLuint brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    // Configuration de la texture
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Création locale du FBO/RBO
    GLuint fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    // Rendu
    GLint oldVp[4];
    glGetIntegerv(GL_VIEWPORT, oldVp);
    glViewport(0, 0, 512, 512);
    
    brdfProg->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    // Restauration
    glViewport(oldVp[0], oldVp[1], oldVp[2], oldVp[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);

    return brdfLUTTexture;
}



void CubemapRender::applyFilter(Program *filterProg, Cubemap target){
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufffer;
    glGenRenderbuffers(1, &depthBufffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, target.resolution, target.resolution);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufffer);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cerr << "[ERROR]Framebuffer not complete!" << std::endl;
    }    

    // Save current viewport dimmensions and prepare it for cubemap faces
    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    glViewport(0,0, target.resolution, target.resolution);

    filterProg->use();

    GLuint skyLoc = glGetUniformLocation(filterProg->programID, "skybox");
    int current = Texture::getAvailableActivationInt();
    glActiveTexture(GL_TEXTURE0 + current);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.textureID);
    glUniform1i(skyLoc, current);

    filterProg->beforeRender();
    filterProg->updateProjectionMatrix(projection);
    filterProg->updateModelMatrix(glm::mat4(1));

    for(int i = 0; i < 6; i++){
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               target.textureID, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(glm::vec3(0), orientations[i], ups[i]);
        filterProg->updateViewMatrix(view);

        cubeMesh.draw(-1);
    }

    glm::mat4 camProj = Camera::getInstance().getP();
    filterProg->updateProjectionMatrix(camProj);
    filterProg->afterRender();

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);
}


void CubemapRender::renderFromPoint(glm::vec3 point, SystemPBR &pbr, EnvironmentRender &envRender) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    GLuint depthBufffer;
    glGenRenderbuffers(1, &depthBufffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemap.resolution, cubemap.resolution);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufffer);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        std::cerr << "Framebuffer not complete!" << std::endl;
    }    

    GLint m_viewport[4];
    glGetIntegerv( GL_VIEWPORT, m_viewport );
    glViewport(0,0, cubemap.resolution, cubemap.resolution);

    pbr.updateProjectionMatrix(projection);
    
    for(int i=0; i<6; i++){
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap.textureID, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto dir = orientations[i];
        
        glm::mat4 view = glm::lookAt(point, point + dir, ups[i]);
        
        pbr.update(view, true);
        envRender.renderSkybox(view);
    }
    
    glm::mat4 camProjection = Camera::getInstance().getP();
    pbr.updateProjectionMatrix(camProjection);

    glViewport(m_viewport[0],m_viewport[1], m_viewport[2], m_viewport[3]);
    glDeleteRenderbuffers(1, &depthBufffer);
    glDeleteFramebuffers(1, &fbo);
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

