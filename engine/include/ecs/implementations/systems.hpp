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
    static Drawable loadMesh(char *filePath);
};

class LightRender: public System {
    public:
    void update();
};

class PBRrender: public System {
    private:
    friend LightRender;
    static PBR* pbrProgPtr;
    GLuint mIrradianceMapID = 0; 

    public:
    static void initPBR();
    void update(glm::mat4 &view);
    void setIrradianceMap(GLuint cubemapTextureID) {
        mIrradianceMapID = cubemapTextureID;
    }
};

class CubemapRender {
    private:
    glm::mat4 projection;
    glm::vec3 orientations[6];
    glm::vec3 ups[6];
    public:
    Cubemap cubemap;
    CubemapRender(int res);
    void renderFromPoint(glm::vec3 point, Render *render, PBRrender *pbr);
    void applyFilter(Program *filterProg, Cubemap target);
    void unwrapOctaProj(GLuint &textureID, int resolution, Skybox *skyboxProgPtr);
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
    GLuint sphereVAO, quadVAO, rayVAO;
    int sphereIndexCount, quadIndexCount;

    public:
    void init();
    void update();
};

class CustomSystem: public System {
    public:
    void update(float deltaTime);
};
