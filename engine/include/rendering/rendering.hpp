#pragma once

#include <memory>

#include <engine/include/rendering/common.hpp>
#include <engine/include/ecs/base/system.hpp>
#include <engine/include/ecs/ecsManager.hpp>


class ProgSkybox: public Program{
    public:
    Cubemap cubemap;
    ProgSkybox(Cubemap sky);
    
    void beforeRender() override;
    void afterRender() override;
};
class ProgIrradiance:public Program{
    public:
    ProgIrradiance();
    
};
class ProgPrefilter:public Program{
    public:
    ProgPrefilter();
};

class ProgBRDF:public Program{
    public:
    ProgBRDF();
};

class ProgCubemap: public Program {
    public:
    GLuint textureID;

    ProgCubemap();

    void beforeRender() override;
};

class ProgPBR: public Program{
    private:
    GLuint albedoLocation, metallicLocation, roughnessLocation, aoLocation, hasTextureLocation, indensiteScaleLightLocation;
    GLuint albedoTexLocation, metallicTexLocation, roughnessTexLocation, aoTexLocation, normalTexLocation;
    GLuint hasAlbedoMapLocation, hasNormalMapLocation, hasMetallicMapLocation, hasRoughnessMapLocation, hasAoMapLocation;
    

    public:
    ProgPBR();
    void updateGUI() override;
    void updateMaterial(Material &value);

    void updateLightCount(int count);
    void updateLightPosition(int lightIndex, glm::vec3 position);
    void updateLightColor(int lightIndex, glm::vec3 color);
    
};

void save_PPM_file(int width, int height, const std::string& filename);




// Separate clearly storage class from shader programs from systems

class SystemLight: public System {
    public:
    void update();
};

class SystemPBR: public System {
    protected:
    friend SystemLight;
    static ProgPBR* pbrProgPtr;
    GLuint mIrradianceMapID = 0; 
    GLuint mPrefilterMapID = 0; 
    GLuint mBrdfLUTID = 0; 


    public:
    static void initPBR();
    void setupMaps();
    void updateProjectionMatrix(glm::mat4 &proj);
    virtual void update(glm::mat4 &view, bool isCubemapRender = false);
    void setIrradianceMap(GLuint cubemapTextureID) {mIrradianceMapID = cubemapTextureID;}
    void setPrefilterMap(GLuint cubemapTextureID) {mPrefilterMapID = cubemapTextureID;}
    void setBrdfLUT(GLuint TextureID) {mBrdfLUTID = TextureID;}
};

class SystemAnimatedPBR: public SystemPBR {
    public:
    void update(glm::mat4 &view, float deltaTime);
    static void loadMesh(char *directory, char *fileName, AnimatedDrawable &res, Material &mat);
};

class EnvironmentRender {
    private:
    Drawable skyboxDraw;
    Transform skyboxTransform;
    std::unique_ptr<ProgSkybox> skyboxProg;
    public:
    EnvironmentRender(ecsManager* ecs);
    void renderSkybox(const glm::mat4 &view);
};

class CubemapRender {
    private:
    glm::mat4 projection;
    glm::vec3 orientations[6];
    glm::vec3 ups[6];
    Drawable cubeMesh;
    public:
    Cubemap cubemap;
    CubemapRender(int res);
    void renderFromPoint(glm::vec3 point, SystemPBR &pbr, EnvironmentRender &envRender);
    void applyFilter(Program *filterProg, Cubemap target);
    void applyPrefilter(Program *filterProg, Cubemap prefilterMap);
    GLuint TwoDLUT(Program *brdfProg);
};

class RenderServer {
    public:
    static RenderServer& getInstance() {
        static RenderServer instance;
        return instance;
    }
    void init(ecsManager& ecs);
    void reset();
    void update(glm::mat4 &view, float deltaTime);

    private:
    RenderServer() = default;
    ecsManager* ecs = nullptr;
    std::shared_ptr<SystemPBR> pbrRenderSystem;
    std::shared_ptr<SystemAnimatedPBR> animatedPbrRenderSystem;
    std::shared_ptr<SystemLight> lightRenderSystem;
    std::shared_ptr<EnvironmentRender> environmentRender;


    // RenderServer(const RenderServer&) = delete;
    // RenderServer& operator=(const RenderServer&) = delete;
    // RenderServer(RenderServer&&) = delete;
    // RenderServer& operator=(RenderServer&&) = delete;
};
