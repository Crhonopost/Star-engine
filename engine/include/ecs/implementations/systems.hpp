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

class CustomSystem: public System {
    public:
    void update(float deltaTime);
};
