#include <engine/include/spatial.hpp>
#include <engine/include/rendering.hpp>
#include <engine/include/input.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/ecs/implementations/systems.hpp>

#include <iostream>

void initScene(SpatialNode &root, ecsManager &ecs){
    ///////////////////////////// programs
    Texture::generateTextures(4);
    Program baseProg("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
    baseProg.initTexture("../assets/images/planets/tex_sun.jpg", "tex\0");


    Program mountainProg("shaders/vertex_shader.glsl", "shaders/fragment_shader_mountain.glsl");
    mountainProg.initTexture("../assets/images/planets/tex_sun.jpg", "texGrass\0");
    mountainProg.initTexture("../assets/images/rock.png", "texRock\0");
    mountainProg.initTexture("../assets/images/HeightMap.png", "heightMap\0");

    Program::programs.push_back(baseProg);
    Program::programs.push_back(mountainProg);




    ////////////////////////////// mountain
    auto mountainEntity = ecs.CreateEntity();
    const int sideLength = 100;
    auto mountainDraw = Render::generatePlane(sideLength,256);
    mountainDraw.program = &Program::programs[1];
    Transform mountainTransform;
    CollisionShape mountainShape;
    mountainShape.shapeType = PLANE;
    mountainShape.plane.normal = glm::vec3(0,1,0);
    RigidBody mountainBody;
    mountainBody.isStatic = true;
    ecs.AddComponent(mountainEntity, mountainDraw);
    ecs.AddComponent(mountainEntity, mountainTransform);
    ecs.AddComponent(mountainEntity, mountainShape);
    ecs.AddComponent(mountainEntity, mountainBody);



    

    ///////////////////////////// sun
    auto sunEntity = ecs.CreateEntity();
    auto sunDraw = Render::generateSphere(0.5f);
    sunDraw.program = &Program::programs[0];
    Transform sunTransform;
    sunTransform.translate(glm::vec3(5,0,0));
    RigidBody sunBody;
    // sunBody.velocity = glm::vec3(1,1,0) * 2.f;
    sunBody.restitutionCoef = 1.0f;
    sunBody.weight = 3.f;
    CollisionShape sunShape;
    sunShape.shapeType = SPHERE;
    sunShape.sphere.radius = 1.f;
    
    CustomBehavior sunBehavior;
    sunBehavior.update = [sunEntity, &ecs](float deltaTime) {
        auto actions = InputManager::getInstance().getActions();

        const glm::vec3 left = glm::normalize(glm::cross(glm::vec3(0,1,0), Camera::getInstance().camera_target));
        const glm::vec3 forward = glm::normalize(glm::cross(left, glm::vec3(0,1,0)));

        const float speed = 10.0f;
        const float jumpStrength = 15.0f;
        auto &sunBody = ecs.GetComponent<RigidBody>(sunEntity);
        glm::vec3 inputVelocity(0);
        if (actions[InputManager::ActionEnum::ACTION_FORWARD].pressed)
            inputVelocity += forward;
        if (actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)
            inputVelocity -= forward;
        if (actions[InputManager::ActionEnum::ACTION_LEFT].pressed)
            inputVelocity += left;
        if (actions[InputManager::ActionEnum::ACTION_RIGHT].pressed)
            inputVelocity -= left;
        
            
        if(inputVelocity.x + inputVelocity.z != 0.f){
            inputVelocity = glm::normalize(inputVelocity) * speed;
            sunBody.velocity = glm::vec3(inputVelocity.x, sunBody.velocity.y, inputVelocity.z);
        }

        if (actions[InputManager::ActionEnum::ACTION_JUMP].pressed)
            sunBody.velocity.y = jumpStrength;
    };

    Drawable lowerRes = Render::generatePlane(1, 2);
    auto lowerResEntity = ecs.CreateEntity();
    ecs.AddComponent<Drawable>(lowerResEntity, lowerRes);
    sunDraw.lodLower = &ecs.GetComponent<Drawable>(lowerResEntity);
    sunDraw.switchDistance = 15;

    ecs.AddComponent(sunEntity, sunDraw);
    ecs.AddComponent(sunEntity, sunTransform);
    ecs.AddComponent(sunEntity, sunBehavior);
    ecs.AddComponent(sunEntity, sunBody);
    ecs.AddComponent(sunEntity, sunShape);


    auto rayTestEntity = ecs.CreateEntity();
    Transform rayTransform;
    CollisionShape rayShape;
    rayShape.shapeType = RAY;
    rayShape.ray.length = 1.5;
    rayShape.ray.ray_direction = glm::vec3(0,-1,0);
    
    ecs.AddComponent<Transform>(rayTestEntity, rayTransform);
    ecs.AddComponent<CollisionShape>(rayTestEntity, rayShape);


    /////////////////////////////// collision debug
    auto otherEntity = ecs.CreateEntity();
    Transform otherTransform;
    otherTransform.translate(glm::vec3(0,5,0));
    CollisionShape otherCollision;
    otherCollision.shapeType = SPHERE;
    otherCollision.sphere.radius = 1.2;
    ecs.AddComponent<Transform>(otherEntity, otherTransform);
    ecs.AddComponent<CollisionShape>(otherEntity, otherCollision);

    

    

    
    ///////////////////////////// camera
    auto cameraEntity = ecs.CreateEntity();
    CustomBehavior cameraUpdate;
    cameraUpdate.update = [sunEntity, &ecs](float deltaTime){
        auto actions = InputManager::getInstance().getActions();

        glm::vec3 sunPos = ecs.GetComponent<Transform>(sunEntity).getLocalPosition();
        glm::vec3 sunVelocity = ecs.GetComponent<RigidBody>(sunEntity).velocity;

        glm::vec3 newTarget = glm::normalize(sunPos + glm::vec3(sunVelocity.x, 0, sunVelocity.z) - Camera::getInstance().camera_position);

        Camera::getInstance().camera_target = glm::mix(Camera::getInstance().camera_target, newTarget, deltaTime);

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
    Camera::getInstance().locked = true;
    Camera::getInstance().camera_position = glm::vec3(0,10,0);


    auto rootEntity = ecs.CreateEntity();
    Transform rootTransform;
    ecs.AddComponent(rootEntity, rootTransform);
    root.transform = &ecs.GetComponent<Transform>(rootEntity);
    
    std::unique_ptr<SpatialNode> sunNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(sunEntity));
    std::unique_ptr<SpatialNode> mountainNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(mountainEntity));
    std::unique_ptr<SpatialNode> rayNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(rayTestEntity));
    std::unique_ptr<SpatialNode> otherNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(otherEntity));

    
    sunNode->AddChild(std::move(rayNode));
    root.AddChild(std::move(sunNode));
    root.AddChild(std::move(mountainNode));
    root.AddChild(std::move(otherNode));
}