#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <engine/include/program.hpp>
#include <engine/include/spatial.hpp>

using std::vector;
using namespace spatial;

struct SceneGraph : public SpatialNode{
    vector<Program> programs;
    bool rotatingScene = false;
    float sceneRotationAngle = 0;

    void init();

    void update(double);
    void render();

    void clear();
};

