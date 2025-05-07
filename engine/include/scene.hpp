#include <engine/include/spatial.hpp>
#include <engine/include/rendering.hpp>
#include <engine/include/input.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/ecs/implementations/systems.hpp>


#include <iostream>

Entity generateSpherePBR(ecsManager &ecs, float radius, glm::vec3 position){
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

Entity generatePlanet(ecsManager &ecs, glm::vec3 position, float radius){
    auto sphereEntity = ecs.CreateEntity();
    auto sphereDraw = Render::generateSphere(radius);
    auto sphereMaterial = Material();

    auto sphereRigidBody = RigidBody();
    sphereRigidBody.isStatic = true;
    auto sphereCollisionShape = CollisionShape();
    sphereCollisionShape.shapeType = SPHERE;
    sphereCollisionShape.sphere.radius = radius;

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
    ecs.AddComponent(sphereEntity, sphereRigidBody);
    ecs.AddComponent(sphereEntity, sphereCollisionShape);

    return sphereEntity;
}

Entity generateGravityArea(ecsManager &ecs, glm::vec3 position, float radius, Entity playerEntity){
    auto entity = ecs.CreateEntity();
    auto collisionShape = CollisionShape();
    auto collisionBehavior = CustomBehavior();
    auto areaStorage = CustomVar();

    areaStorage.bools.push_back(false); // If player is in gravity range

    Transform sphereTransform;
    sphereTransform.translate(position);

    collisionShape.shapeType = SPHERE;
    collisionShape.sphere.radius = radius;
    collisionShape.layer = 0;
    collisionShape.mask = CollisionShape::PLAYER_LAYER;

    collisionBehavior.update = [playerEntity, entity, &ecs](float deltaTime){
        if(ecs.GetComponent<CollisionShape>(entity).isColliding){
            glm::vec3 gravDir = ecs.GetComponent<Transform>(entity).getGlobalPosition() - ecs.GetComponent<Transform>(playerEntity).getGlobalPosition();
            ecs.GetComponent<RigidBody>(playerEntity).gravityDirection = glm::normalize(gravDir);
            ecs.GetComponent<CustomVar>(entity).bools[0] = true;
        } else if(ecs.GetComponent<CustomVar>(entity).bools[0]){
            ecs.GetComponent<RigidBody>(playerEntity).gravityDirection = glm::vec3(0);
            ecs.GetComponent<CustomVar>(entity).bools[0] = false;
        }
    };

    ecs.AddComponent(entity, collisionShape);
    ecs.AddComponent(entity, collisionBehavior);
    ecs.AddComponent(entity, areaStorage);
    ecs.AddComponent(entity, sphereTransform);

    return entity;
}

void initScene(SpatialNode &root, ecsManager &ecs){
    Program::programs.push_back(std::make_unique<PBR>());    
    ///////////////////////////// sun
    auto sunEntity = generateSpherePBR(ecs, 0.75f, {2, 1, 0});;
    ecs.SetEntityName(sunEntity, "Sun");
    auto &sunDraw = ecs.GetComponent<Drawable>(sunEntity);
    RigidBody sunBody;
    // sunBody.velocity = glm::vec3(1,1,0) * 2.f;
    sunBody.restitutionCoef = 1.0f;
    sunBody.weight = 3.f;
    CollisionShape sunShape;
    sunShape.shapeType = SPHERE;
    sunShape.sphere.radius = 1.f;
    sunShape.layer = CollisionShape::ENV_LAYER | CollisionShape::PLAYER_LAYER;
    
    CustomBehavior sunBehavior;
    sunBehavior.update = [sunEntity, &ecs](float deltaTime) {
        auto actions = InputManager::getInstance().getActions();

        glm::vec3 normal = -ecs.GetComponent<RigidBody>(sunEntity).gravityDirection;
        if(normal.length() == 0.f){
            normal = glm::vec3(0,1,0);
        }
        const glm::vec3 left = glm::normalize(glm::cross(normal, {0,1,0}));
        const glm::vec3 forward = glm::normalize(glm::cross(left, normal));

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
        
            
        if(inputVelocity.x != 0.f || inputVelocity.y != 0.f || inputVelocity.x != 0.f){
            inputVelocity = glm::normalize(inputVelocity) * speed;
            sunBody.velocity = inputVelocity;//glm::vec3(inputVelocity.x, sunBody.velocity.y, inputVelocity.z);
        }

        if (actions[InputManager::ActionEnum::ACTION_JUMP].pressed)
            sunBody.velocity = -sunBody.gravityDirection * jumpStrength;
    };

    Drawable lowerRes = Render::generatePlane(1, 2);
    auto lowerResEntity = ecs.CreateEntity();
    ecs.AddComponent<Drawable>(lowerResEntity, lowerRes);
    sunDraw.lodLower = &ecs.GetComponent<Drawable>(lowerResEntity);
    sunDraw.switchDistance = 15;

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


    auto planetEntity = generatePlanet(ecs, {14,0,14}, 20.f);
    auto planetGravity = generateGravityArea(ecs, glm::vec3(0), 60.f, sunEntity);


    // auto cabaneEntity = ecs.CreateEntity();
    // Transform cabaneTransform;
    // Drawable cabaneDraw = Render::loadMesh("../assets/meshes/cabane.glb");
    // Material cabaneMaterial;
    // cabaneMaterial.albedo = {0.5f,0.5f,0.5f};
    // ecs.AddComponent<Transform>(cabaneEntity, cabaneTransform);
    // ecs.AddComponent<Drawable>(cabaneEntity, cabaneDraw);
    // ecs.AddComponent<Material>(cabaneEntity, cabaneMaterial);


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


    // TODO: fix custom programs, when adding mountain: pbr rendering not working ....
    ///////////////////////////// programs
    auto mountainProg = std::make_unique<Program>("shaders/vertex_shader_mountain.glsl", "shaders/fragment_shader_mountain.glsl");
    mountainProg->initTexture("../assets/images/grass.png", "texGrass\0");
    mountainProg->initTexture("../assets/images/rock.png", "texRock\0");
    mountainProg->initTexture("../assets/images/HeightMap.png", "heightMap\0");


    // OOBB collision test
    auto b1Entity = ecs.CreateEntity();
    ecs.SetEntityName(b1Entity, "b1");
    auto b1Collision = CollisionShape();
    b1Collision.shapeType = OOBB;
    b1Collision.oobb = Oobb();
    
    auto b1Transform = Transform();
    b1Transform.translate({0,20,0});
    ecs.AddComponent<CollisionShape>(b1Entity, b1Collision);
    ecs.AddComponent<Transform>(b1Entity, b1Transform);
    
    auto b2Entity = ecs.CreateEntity();
    ecs.SetEntityName(b2Entity, "b2");
    auto b2Collision = CollisionShape();
    b2Collision.shapeType = OOBB;
    b2Collision.oobb = Oobb();
    
    auto b2Transform = Transform();
    b2Transform.translate({0,20,0});
    ecs.AddComponent<CollisionShape>(b2Entity, b2Collision);
    ecs.AddComponent<Transform>(b2Entity, b2Transform);
    

    
    ///////////////////////////// camera
    auto cameraEntity = ecs.CreateEntity();
    ecs.SetEntityName(cameraEntity, "Camera player default");
    CustomBehavior cameraUpdate;
    Transform cameraTransform;
    CameraComponent cameraComponent;
    cameraComponent.needActivation = true;
    cameraUpdate.update = [sunEntity, cameraEntity, &ecs](float deltaTime){
        auto actions = InputManager::getInstance().getActions();

        RigidBody& sunRigid = ecs.GetComponent<RigidBody>(sunEntity);

        Transform &sunTransform = ecs.GetComponent<Transform>(sunEntity);
        Transform &camTransform = ecs.GetComponent<Transform>(cameraEntity);

        camTransform.setLocalPosition(-sunRigid.gravityDirection * 10.f);
        glm::vec3 direction = sunTransform.getGlobalPosition() - camTransform.getGlobalPosition();
        direction = glm::normalize(sunRigid.gravityDirection);
        camTransform.setLocalRotation(Camera::lookAt(direction));
    };
    ecs.AddComponent(cameraEntity, cameraUpdate);
    ecs.AddComponent(cameraEntity, cameraTransform);
    ecs.AddComponent(cameraEntity, cameraComponent);


    auto rootEntity = ecs.CreateEntity();
    Transform rootTransform;
    ecs.AddComponent(rootEntity, rootTransform);
    root.transform = &ecs.GetComponent<Transform>(rootEntity);
    
    std::unique_ptr<SpatialNode> sunNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(sunEntity));
    std::unique_ptr<SpatialNode> sunCameraNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(cameraEntity));
    std::unique_ptr<SpatialNode> rayNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(rayTestEntity));
    std::unique_ptr<SpatialNode> b1Node = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(b1Entity));
    std::unique_ptr<SpatialNode> b2Node = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(b2Entity));
    std::unique_ptr<SpatialNode> otherNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(otherEntity));
    std::unique_ptr<SpatialNode> planetNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(planetEntity));
    std::unique_ptr<SpatialNode> planetGravityNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(planetGravity));

    
    sunNode->AddChild(std::move(rayNode));
    sunNode->AddChild(std::move(sunCameraNode));
    root.AddChild(std::move(sunNode));
    root.AddChild(std::move(otherNode));
    root.AddChild(std::move(b1Node));
    root.AddChild(std::move(b2Node));
    // root.AddChild(std::move(cabaneNode));
    planetNode->AddChild(std::move(planetGravityNode));
    root.AddChild(std::move(planetNode));
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
        auto ent = generateSpherePBR(ecs, 0.75f, glm::vec3(-5 + i*2, 0, 0));
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
