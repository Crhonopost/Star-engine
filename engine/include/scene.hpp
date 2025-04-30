#include <engine/include/spatial.hpp>
#include <engine/include/rendering.hpp>
#include <engine/include/input.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/ecs/implementations/systems.hpp>


#include <iostream>

Entity generateSpherePBR(ecsManager &ecs, Program *pbrProg, float radius, glm::vec3 position){
    auto sphereEntity = ecs.CreateEntity();
    auto sphereDraw = Render::generateSphere(radius);
    auto sphereMaterial = Material();

    sphereMaterial.albedoTex = &Texture::loadTexture("../assets/images/PBR/woods/Albedo.jpg");
    sphereMaterial.normalTex = &Texture::loadTexture("../assets/images/PBR/woods/Normal.jpg");
    sphereMaterial.metallicTex = &Texture::loadTexture("../assets/images/PBR/woods/Specular.jpg");
    sphereMaterial.roughnessTex = &Texture::loadTexture("../assets/images/PBR/woods/Roughness.jpg");
    sphereMaterial.aoTex = &Texture::loadTexture("../assets/images/PBR/woods/AO.jpg");

    Transform sphereTransform;
    sphereTransform.translate(position);

    ecs.AddComponent(sphereEntity, sphereDraw);
    ecs.AddComponent(sphereEntity, sphereMaterial);
    ecs.AddComponent(sphereEntity, sphereTransform);

    return sphereEntity;
}

Entity createLightSource(ecsManager &ecs, glm::vec3 position, glm::vec3 color){
    auto otherEntity = ecs.CreateEntity();
    Transform otherTransform;
    otherTransform.translate(position);
    Light lightSource;
    lightSource.color = color;
    
    ecs.AddComponent<Transform>(otherEntity, otherTransform);
    ecs.AddComponent<Light>(otherEntity, lightSource);

    return otherEntity;
}

void initScene(SpatialNode &root, ecsManager &ecs){
    ///////////////////////////// programs
    auto mountainProg = std::make_unique<Program>("shaders/vertex_shader_mountain.glsl", "shaders/fragment_shader_mountain.glsl");
    mountainProg->initTexture("../assets/images/grass.png", "texGrass\0");
    mountainProg->initTexture("../assets/images/rock.png", "texRock\0");
    mountainProg->initTexture("../assets/images/HeightMap.png", "heightMap\0");

    Program::programs.push_back(std::make_unique<PBR>());
    Program::programs.push_back(std::move(mountainProg));




    ////////////////////////////// mountain
    auto mountainEntity = ecs.CreateEntity();
    const int sideLength = 100;
    auto mountainDraw = Render::generatePlane(sideLength,256);
    CustomProgram mountainCustomProg(Program::programs[1].get());
    Transform mountainTransform;
    CollisionShape mountainShape;
    mountainShape.shapeType = PLANE;
    mountainShape.plane.normal = glm::vec3(0,1,0);
    RigidBody mountainBody;
    mountainBody.isStatic = true;
    ecs.AddComponent(mountainEntity, mountainDraw);
    ecs.AddComponent(mountainEntity, mountainCustomProg);
    ecs.AddComponent(mountainEntity, mountainTransform);
    ecs.AddComponent(mountainEntity, mountainShape);
    ecs.AddComponent(mountainEntity, mountainBody);



    

    ///////////////////////////// sun
    auto sunEntity = ecs.CreateEntity();
    auto sunDraw = Render::generateSphere(0.5f);
    auto sunMaterial = Material();
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
    ecs.AddComponent(sunEntity, sunMaterial);
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
    
    ecs.AddComponent(rayTestEntity, rayTransform);
    ecs.AddComponent(rayTestEntity, rayShape);


    /////////////////////////////// collision debug
    auto otherEntity = ecs.CreateEntity();
    Transform otherTransform;
    otherTransform.translate(glm::vec3(0,5,0));
    Light lightSource;
    lightSource.color = glm::vec3(0,1,0);
    CollisionShape otherCollision;
    otherCollision.shapeType = SPHERE;
    otherCollision.sphere.radius = 1.2;
    ecs.AddComponent(otherEntity, otherTransform);
    ecs.AddComponent(otherEntity, lightSource);
    ecs.AddComponent(otherEntity, otherCollision);




    /////////////////////////////////// Mesh test
    auto barrierEntity = ecs.CreateEntity();
    ecs.SetEntityName(barrierEntity, "Barrier");
    Transform barrierTransform;
    barrierTransform.translate(glm::vec3(0,2,0));
    Drawable barrierDrawable = Render::loadMesh("../assets/meshes/barrier_1x1x1.gltf");
    Material barrierMaterial;
    CollisionShape barrierCollision;
    barrierCollision.shapeType = SPHERE;
    barrierCollision.sphere.radius = 1.2;
    ecs.AddComponent(barrierEntity, barrierTransform);
    ecs.AddComponent(barrierEntity, barrierDrawable);
    ecs.AddComponent(barrierEntity, barrierMaterial);
    ecs.AddComponent(barrierEntity, barrierCollision);

    

    

    
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
    std::unique_ptr<SpatialNode> barrierNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(barrierEntity));

    
    sunNode->AddChild(std::move(rayNode));
    root.AddChild(std::move(sunNode));
    root.AddChild(std::move(mountainNode));
    root.AddChild(std::move(otherNode));
    root.AddChild(std::move(barrierNode));
}




float totalTime = 0;
void pbrScene(SpatialNode &root, ecsManager &ecs){
    auto cameraEntity = ecs.CreateEntity();
    CustomBehavior cameraUpdate;
    cameraUpdate.update = [](float deltaTime){
        auto actions = InputManager::getInstance().getActions();

        const glm::vec3 left = glm::normalize(glm::cross(glm::vec3(0,1,0), Camera::getInstance().camera_target));
        const glm::vec3 forward = glm::normalize(glm::cross(left, glm::vec3(0,1,0)));

        const float speed = 10.0f;
        // glm::vec3 inputVelocity(0);
        // if (actions[InputManager::ActionEnum::ACTION_FORWARD].pressed)
        //     inputVelocity += forward;
        // if (actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)
        //     inputVelocity -= forward;
        // if (actions[InputManager::ActionEnum::ACTION_LEFT].pressed)
        //     inputVelocity += left;
        // if (actions[InputManager::ActionEnum::ACTION_RIGHT].pressed)
        //     inputVelocity -= left;

        // if(inputVelocity.length() != 0)
        //     Camera::getInstance().camera_position += glm::normalize(inputVelocity) * deltaTime * speed;
    };
    ecs.AddComponent(cameraEntity, cameraUpdate);
    Camera::editor = false;
    Camera::getInstance().camera_position = glm::vec3(0,1,10);
    Camera::editor = true;
    Camera::getInstance().camera_position = glm::vec3(0,1,10);   
    
    auto rootEntity = ecs.CreateEntity();
    Transform rootTransform;
    ecs.AddComponent(rootEntity, rootTransform);
    root.transform = &ecs.GetComponent<Transform>(rootEntity);

    CustomBehavior continuousRotation;
    continuousRotation.update = [&root](float deltaTime){
        root.transform->rotate(glm::vec3(0, deltaTime * 10.f, 0));
    };
    ecs.AddComponent<CustomBehavior>(rootEntity, continuousRotation);

    Program::programs.push_back(std::make_unique<PBR>());
    for(int i=0; i<5; i++){
        auto ent = generateSpherePBR(ecs, Program::programs[0].get(), 0.75f, glm::vec3(-5 + i*2, 0, 0));
        std::unique_ptr<SpatialNode> sphereNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(ent));
        root.AddChild(std::move(sphereNode));
    }
    
    // auto light1 = createLightSource(ecs, glm::vec3(1,5,-6), glm::vec3(1));
    // ecs.SetEntityName(light1, "Center top light");
    std::vector<Entity> staticLights;
    std::vector<glm::vec3> lightPositions = {
        {-3, 3, -5},
        {-3, 3, 3},
        {0, 1,  5},
        {3, -3,  -2}
    };

    for (int i = 0; i < lightPositions.size(); ++i) {
        auto light = createLightSource(ecs, lightPositions[i], glm::vec3(1));
        ecs.SetEntityName(light, "Static Light " + std::to_string(i + 1));
        staticLights.push_back(light);
    }

    auto movingLight = createLightSource(ecs, glm::vec3(5,0,2), glm::vec3(1));
    ecs.SetEntityName(movingLight, "Y moving light");
    CustomBehavior oscilatingLight;
    oscilatingLight.update = [movingLight, &ecs](float deltaTime){
        totalTime += deltaTime;
        auto &transfo = ecs.GetComponent<Transform>(movingLight);
        float direction = cos(totalTime); 
        transfo.translate({0,direction * deltaTime * 10.f,0});
    };
    ecs.AddComponent<CustomBehavior>(movingLight, oscilatingLight);
}
