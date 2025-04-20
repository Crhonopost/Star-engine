#pragma once

#include <engine/include/ecs/base/system.hpp>
#include <engine/include/ecs/implementations/components.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/rendering.hpp>


extern ecsManager ecs;

class Render: public System {
    public:
    void update();

    static Drawable generateSphere(float radius);
    static Drawable generatePlane(float sideLength, int nbOfVerticesSide);
    static Drawable generateInwardCube(float sideLength, int nbOfVerticesSide);
    static Drawable loadMesh(char *filePath);
};

class PBRrender: public System {
    private:
    PBR pbrProg;
    public:
    PBRrender();
    void update();
};


class LightRender: public System {
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
