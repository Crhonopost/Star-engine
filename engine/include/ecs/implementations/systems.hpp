#pragma once

#include <engine/include/ecs/base/system.hpp>
#include <engine/include/ecs/implementations/components.hpp>
#include <engine/include/ecs/ecsManager.hpp>

extern ecsManager ecs;

class Render: public System {
    public:
    void update();

    static Drawable generateSphere(float radius);
    static Drawable generatePlane(float sideLength, int nbOfVerticesSide);
};

class PhysicSystem: public System {
    private:
        void broadPhase();
        void narrowPhase();
        void solver();
    public:
    void update(float deltaTime);
};

class CustomSystem: public System {
    public:
    void update(float deltaTime);
};
