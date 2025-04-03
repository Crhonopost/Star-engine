#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <engine/include/program.hpp>
#include <engine/include/spatial.hpp>
#include "ecs/implementations/systems.hpp"

using std::vector;
using namespace spatial;

SpatialNode initScene(ecsManager &ecs){
    auto actions = InputManager::getInstance().getActions();

    ///////////////////////////// programs
    Program baseProg("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
    Texture sunTexture = baseProg.loadTexture("../assets/images/planets/tex_sun.jpg", "tex");


    Program mountainProg("shaders/vertex_shader_mountain.glsl", "shaders/fragment_shader.glsl");
    Texture heightMap = mountainProg.loadTexture("../assets/images/HeightMap.png", "heightMap");
    Texture grassTex = mountainProg.loadTexture("../assets/images/grass.png", "tex");
    // Texture rockTex = mountainProg.loadTexture("../assets/images/rock.png", "texRock");

    Program::programs.push_back(baseProg);
    Program::programs.push_back(mountainProg);
    

    ///////////////////////////// sun
    auto sunEntity = ecs.CreateEntity();
    auto sunDraw = Render::generateSphere(0.5f);
    sunDraw.programIdx = 0;
    sunDraw.textures.push_back(sunTexture);
    Transform sunTransform;
    
    CustomBehavior sunBehavior;
    sunBehavior.update = [&sunEntity, &actions, &ecs](float deltaTime) {

        const glm::vec3 left = glm::cross(glm::vec3(0,1,0), Camera::getInstance().camera_target);
        const glm::vec3 forward = glm::cross(left, glm::vec3(0,1,0));
        
        const float speed = 10.0f;
        if (actions[InputManager::ActionEnum::ACTION_FORWARD].pressed)
            ecs.GetComponent<Transform>(sunEntity).translate(forward * speed * deltaTime);
        if (actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)
            ecs.GetComponent<Transform>(sunEntity).translate(-forward * speed * deltaTime);
        if (actions[InputManager::ActionEnum::ACTION_LEFT].pressed)
            ecs.GetComponent<Transform>(sunEntity).translate(left * speed * deltaTime);
        if (actions[InputManager::ActionEnum::ACTION_RIGHT].pressed)
            ecs.GetComponent<Transform>(sunEntity).translate(-left * speed * deltaTime);
    };

    ecs.AddComponent(sunEntity, sunDraw);
    ecs.AddComponent(sunEntity, sunTransform);
    ecs.AddComponent(sunEntity, sunBehavior);

    
    
    ////////////////////////////// mountain
    auto mountainEntity = ecs.CreateEntity();
    auto mountainDraw = Render::generatePlane(100,100,400);
    mountainDraw.programIdx = 1;
    mountainDraw.textures.push_back(heightMap);
    mountainDraw.textures.push_back(grassTex);
    Transform mountainTransform;
    mountainTransform.translate(glm::vec3(0,-20,0));
    ecs.AddComponent(mountainEntity, mountainDraw);
    ecs.AddComponent(mountainEntity, mountainTransform);
    

    

    
    ///////////////////////////// camera
    auto cameraEntity = ecs.CreateEntity();
    CustomBehavior cameraUpdate;
    cameraUpdate.update = [&sunEntity,  &actions, &ecs](float deltaTime){
        glm::vec3 sunPos = ecs.GetComponent<Transform>(sunEntity).getLocalPosition();
        glm::vec3 newTarget = glm::normalize(sunPos - Camera::getInstance().camera_position);

        Camera::getInstance().camera_target = newTarget;

        if (actions[InputManager::ActionEnum::ACTION_LOCK_POSITION].clicked)
            Camera::getInstance().locked = !Camera::getInstance().locked;

        if(!Camera::getInstance().locked){
            glm::vec3 newPos = sunPos - newTarget * 10.f;
            newPos.y = sunPos.y + 5.0f;
            Camera::getInstance().camera_position = glm::mix(Camera::getInstance().camera_position, newPos, deltaTime);
        }
        // Camera::getInstance().updateInput(deltaTime);
    };
    ecs.AddComponent(cameraEntity, cameraUpdate);


    auto root = spatial::SpatialNode();
    Transform rootTransform;
    root.transform = &rootTransform;
    
    auto sunNode = spatial::SpatialNode();
    sunNode.transform = &ecs.GetComponent<Transform>(sunEntity);
    auto mountainNode = spatial::SpatialNode();        
    mountainNode.transform = &ecs.GetComponent<Transform>(mountainEntity);
    
    root.AddChild(&sunNode);
    root.AddChild(&mountainNode);

    return root;
}

