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
    static Drawable generatePlane(float sideLength, int nbOfVerticesSide);
    static Drawable generateCube(float sideLength, int nbOfVerticesSide, bool inward=false);
    static Drawable loadSimpleMesh(char *path);
    static void loadSimpleMesh(char *directory, char *fileName, Drawable &res, Material &mat);
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
    static void loadMesh(char *directory, char *fileName, AnimatedDrawable &res, Material &mat);
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
    void applyFilter(Program *filterProg, Cubemap target);
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
