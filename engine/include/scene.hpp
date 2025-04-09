#include <engine/include/spatial.hpp>
#include <engine/include/program.hpp>
#include <engine/include/input.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/ecs/implementations/systems.hpp>

void initScene(SpatialNode &root, ecsManager &ecs){
    ///////////////////////////// programs
    Program::generateTextures(4);
    Program baseProg("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
    Texture sunTexture = baseProg.loadTexture("../assets/images/planets/tex_sun.jpg", "tex\0");


    Program mountainProg("shaders/vertex_shader.glsl", "shaders/fragment_shader_mountain.glsl");
    Texture grassTex = mountainProg.loadTexture("../assets/images/grass.png", "texGrass\0");
    Texture rockTex = mountainProg.loadTexture("../assets/images/rock.png", "texRock\0");
    Texture heightMap = mountainProg.loadTexture("../assets/images/HeightMap.png", "heightMap\0");

    Program::programs.push_back(baseProg);
    Program::programs.push_back(mountainProg);




    ////////////////////////////// mountain
    auto mountainEntity = ecs.CreateEntity();
    const int sideLength = 100;
    auto mountainDraw = Render::generatePlane(sideLength,256);
    mountainDraw.programIdx = 1;
    mountainDraw.textures.push_back(grassTex);
    mountainDraw.textures.push_back(rockTex);
    mountainDraw.textures.push_back(heightMap);
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
    sunDraw.programIdx = 0;
    sunDraw.textures.push_back(sunTexture);
    Transform sunTransform;
    sunTransform.translate(glm::vec3(5,10,0));
    RigidBody sunBody;
    sunBody.velocity = glm::vec3(1,1,0) * 10.f;
    sunBody.restitutionCoef = 1.0f;
    sunBody.weight = 1.f;
    CollisionShape sunShape;
    sunShape.shapeType = SPHERE;
    sunShape.sphere.radius = 1.f;
    
    CustomBehavior sunBehavior;
    sunBehavior.update = [sunEntity, &ecs](float deltaTime) {
        auto actions = InputManager::getInstance().getActions();

        const glm::vec3 left = glm::cross(glm::vec3(0,1,0), Camera::getInstance().camera_target);
        const glm::vec3 forward = glm::cross(left, glm::vec3(0,1,0));
        
        const float speed = 10.0f;
        const float jumpStrength = 20.0f;
        auto &sunBody = ecs.GetComponent<RigidBody>(sunEntity);
        glm::vec3 inputVelocity(0, sunBody.velocity.y, 0);
        if (actions[InputManager::ActionEnum::ACTION_FORWARD].pressed)
            inputVelocity += forward;
        if (actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)
            inputVelocity -= forward;
        if (actions[InputManager::ActionEnum::ACTION_LEFT].pressed)
            inputVelocity += left;
        if (actions[InputManager::ActionEnum::ACTION_RIGHT].pressed)
            inputVelocity -= left;
        
        inputVelocity = glm::normalize(inputVelocity) * speed;

        sunBody.velocity = glm::mix(sunBody.velocity, inputVelocity, deltaTime);
        
        if (actions[InputManager::ActionEnum::ACTION_JUMP].pressed)
            sunBody.velocity.y = jumpStrength;
        
        // if(vel.x !=0 || vel.y != 0 || vel.z != 0){
        //     sunBody.velocity = glm::normalize(vel) * speed;
        // } else {
        //     sunBody.velocity = vel;
        // }
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
    
    root.AddChild(std::move(sunNode));
    root.AddChild(std::move(mountainNode));
}