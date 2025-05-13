#pragma once

#include <engine/include/ecs/base/system.hpp>
#include <engine/include/ecs/implementations/components.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/rendering.hpp>


extern ecsManager ecs;

class Render: public System {
    public:
    void update(glm::mat4 &view);

    static Drawable generateSphere(float radius);
    static Drawable generatePlane(float sideLength, int nbOfVerticesSide, bool front = false);
    static Drawable generateCube(float sideLength, int nbOfVerticesSide, bool inward=false);
    static Drawable loadSimpleMesh(char *path);
};

class LightRender: public System {
    public:
    void update();
};

class PBRrender: public System {
    protected:
    friend LightRender;
    static PBR* pbrProgPtr;
    GLuint mIrradianceMapID = 0; 
    GLuint mPrefilterMapID = 0; 
    GLuint mBrdfLUTID = 0; 


    public:
    static void initPBR();
    void setupMaps();
    virtual void update(glm::mat4 &view);
    void setIrradianceMap(GLuint cubemapTextureID) {
        mIrradianceMapID = cubemapTextureID;
    }
    void setPrefilterMap(GLuint cubemapTextureID) {
        mPrefilterMapID = cubemapTextureID;
    }
    void setBrdfLUT(GLuint TextureID) {
        mBrdfLUTID = TextureID;
    }
};


class AnimatedPBRrender: public PBRrender {
    public:
    void update(glm::mat4 &view, float deltaTime);
    static AnimatedDrawable loadMesh(char *filePath);
};

class InfosRender: public System {
    private:
    Program infoProgram;
    
    public:
    InfosRender();
    void update(glm::mat4 &view, glm::mat4 &projection, int mode=0);
    GLuint renderOnFrame(glm::mat4 &view, glm::mat4 &projection, int width, int height, int mode=0);
};

class ProbeManager{
    std::vector<GLuint> textureIDs;
    ProbeProg prog;
    public:
    ProbeManager();
    void initProbes(Render *render, PBRrender *pbr, InfosRender &infosRender, Skybox *skyboxProgPtr);
    void clear();
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
    void renderFromPoint(glm::vec3 point, Render *render, PBRrender *pbr);
    void renderInfosFromPoint(glm::vec3 point, InfosRender &infosRender, int mode);
    void applyFilter(Program *filterProg, Cubemap target);
    int unwrapOctaProj(GLuint &textureID, int resolution, Skybox *skyboxProgPtr);
    void applyPrefilter(Program *filterProg, Cubemap prefilterMap);
    GLuint TwoDLUT(Program *brdfProg);
};


class CameraSystem: public System {
    private:
        std::queue<Entity> cams;
    public:
        void update();
};


class CollisionDetectionSystem: public System {
    private:
        // void broadPhase();
        void narrowPhase();
        
    public: 
        void update(float deltaTime);
};

class PhysicSystem: public System {
    private:
        void solver();
    public:
        void update(float deltaTime);
};

class PhysicDebugSystem: public System {
    private:
    Program program;
    GLuint sphereVAO, quadVAO, rayVAO, boxVAO;
    int sphereIndexCount, quadIndexCount, boxIndexCount;

    public:
    void init();
    void update();
};

class CustomSystem: public System {
    public:
    void update(float deltaTime);
};
