#pragma once

#include <engine/include/ecs/base/system.hpp>
#include <engine/include/ecs/implementations/components.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/rendering/rendering.hpp>

#include <stack>


extern ecsManager ecs;
class CameraSystem: public System {
    private:
        std::stack<Entity> cams;
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
        void accumulateForces();
        // float linearProjectionPercent = 0.8f;
        // float penetrationSlack = 0.1;
        int impulseIteration = 20;
    public:
        void update(float deltaTime);
        static glm::mat3 processInvertInertia(CollisionShape &shape, RigidBody &rigidBody);
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
