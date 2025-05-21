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

    sphereMaterial.albedoTex = &Texture::loadTexture("../assets/images/PBR/oldMetal/Albedo.png");
    sphereMaterial.normalTex = &Texture::loadTexture("../assets/images/PBR/oldMetal/Normal.png");
    sphereMaterial.metallicTex = &Texture::loadTexture("../assets/images/PBR/oldMetal/Albedo.png");
    sphereMaterial.roughnessTex = &Texture::loadTexture("../assets/images/PBR/oldMetal/Roughness.png");
    sphereMaterial.aoTex = &Texture::loadTexture("../assets/images/PBR/oldMetal/AO.png");

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
    
    ecs.AddComponent(otherEntity, otherTransform);
    ecs.AddComponent(otherEntity, lightSource);

    return otherEntity;
}

Entity generatePlanetBody(ecsManager &ecs, glm::vec3 position, float radius){
    auto sphereEntity = ecs.CreateEntity();
    // auto sphereDraw = Render::generateSphere(radius);    

    auto sphereRigidBody = RigidBody();
    sphereRigidBody.type = RigidBody::STATIC;
    auto sphereCollisionShape = CollisionShape();
    sphereCollisionShape.shapeType = SPHERE;
    sphereCollisionShape.sphere.radius = radius;

    // sphereMaterial.albedoTex = &Texture::loadTexture("../assets/images/PBR/woods/Albedo.jpg");
    // sphereMaterial.normalTex = &Texture::loadTexture("../assets/images/PBR/woods/Normal.jpg");
    // sphereMaterial.metallicTex = &Texture::loadTexture("../assets/images/PBR/woods/Albedo.jpg");
    // sphereMaterial.roughnessTex = &Texture::loadTexture("../assets/images/PBR/woods/Roughness.jpg");
    // sphereMaterial.aoTex = &Texture::loadTexture("../assets/images/PBR/woods/AO.jpg");

    Transform sphereTransform;
    sphereTransform.translate(position);

    ecs.AddComponent(sphereEntity, sphereTransform);
    ecs.AddComponent(sphereEntity, sphereRigidBody);
    ecs.AddComponent(sphereEntity, sphereCollisionShape);

    return sphereEntity;
}

Entity generateGravityArea(ecsManager &ecs, glm::vec3 position, float radius, Entity playerEntity){
    auto entity = ecs.CreateEntity();
    auto collisionShape = CollisionShape();
    auto collisionBehavior = CustomBehavior();

    Transform sphereTransform;
    sphereTransform.translate(position);

    collisionShape.shapeType = SPHERE;
    collisionShape.sphere.radius = radius;
    collisionShape.layer = 0;
    collisionShape.mask = CollisionShape::PLAYER_LAYER | CollisionShape::GRAVITY_SENSITIVE_LAYER;

    collisionBehavior.update = [entity, &ecs](float deltaTime){
        for(auto &collidingEntity: ecs.GetComponent<CollisionShape>(entity).collidingEntities){
            ecs.GetComponent<RigidBody>(collidingEntity).setGravityAnchor(ecs.GetComponent<Transform>(entity).getGlobalPosition());
        }
    };

    ecs.AddComponent(entity, collisionShape);
    ecs.AddComponent(entity, collisionBehavior);
    ecs.AddComponent(entity, sphereTransform);

    return entity;
}

Entity generateCrate(ecsManager &ecs, glm::vec3 position){
    auto crateEntity = ecs.CreateEntity();
    Material crateMat;
    Drawable crateDrawable;
    Render::loadSimpleMesh("../assets/meshes", "/Props/crate.glb", crateDrawable, crateMat);
    
    CollisionShape crateShape;
    crateShape.shapeType = OOBB;
    crateShape.oobb.halfExtents = vec3(1.f);
    crateShape.layer |= CollisionShape::GRAVITY_SENSITIVE_LAYER;
    RigidBody crateBody;
    
    Transform crateTransform;
    crateTransform.translate(position);
    ecs.AddComponent(crateEntity, crateTransform);
    ecs.AddComponent(crateEntity, crateShape);
    ecs.AddComponent(crateEntity, crateBody);
    ecs.AddComponent(crateEntity, crateDrawable);
    ecs.AddComponent(crateEntity, crateMat);

    return crateEntity;
}


Entity generateEgg(ecsManager &ecs, SpatialNode *parent, glm::vec3 position){
    auto eggEntity = ecs.CreateEntity();
    Transform eggTransform;
    CollisionShape eggShape;
    RigidBody eggBody;
    
    eggTransform.translate(position);
    eggBody.type = RigidBody::RIGID;
    eggBody.setMass(3);
    eggShape.shapeType = SPHERE;
    eggShape.sphere.radius = 1.f;

    ecs.AddComponent(eggEntity, eggTransform);
    ecs.AddComponent(eggEntity, eggBody);
    ecs.AddComponent(eggEntity, eggShape);

    
    
    Entity eggMeshEntity = ecs.CreateEntity();
    Transform eggMeshTransform;
    eggMeshTransform.setScale(glm::vec3(0.001));
    Drawable eggDrawable;
    Material eggMaterial;
    Render::loadSimpleMesh("../assets/meshes/Props", "/Egg.glb", eggDrawable, eggMaterial);
    ecs.AddComponent(eggMeshEntity, eggMeshTransform);
    ecs.AddComponent(eggMeshEntity, eggDrawable);
    ecs.AddComponent(eggMeshEntity, eggMaterial);
    
    
    std::unique_ptr<SpatialNode> eggNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(eggEntity));
    eggNode->AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(eggMeshEntity)));
    parent->AddChild(std::move(eggNode));
    return eggEntity;
}

Entity generatePlayer(ecsManager &ecs, SpatialNode &parent){
    // Ground check
    auto groundCheckEntity = ecs.CreateEntity();
    Transform rayTransform;
    CollisionShape rayShape;
    rayShape.shapeType = RAY;
    rayShape.ray.length = 5;
    rayShape.ray.ray_direction = glm::vec3(0,-1,0);
    rayShape.layer = 0;
    rayShape.mask = CollisionShape::ENV_LAYER;
    
    ecs.AddComponent(groundCheckEntity, rayTransform);
    ecs.AddComponent(groundCheckEntity, rayShape);


    // Player entity
    auto playerEntity = ecs.CreateEntity();
    ecs.SetEntityName(playerEntity, "Player");
    AnimatedDrawable playerDrawable;
    Material playerMaterial;
    Transform playerTransform;
    ecs.AddComponent(playerEntity, playerTransform);

    AnimatedPBRrender::loadMesh("../assets/meshes/Player", "/Run.glb", playerDrawable, playerMaterial);
    ecs.AddComponent(playerEntity, playerDrawable);
    ecs.AddComponent(playerEntity, playerMaterial);



    // auto &playerDraw = ecs.GetComponent<Drawable>(playerEntity);
    RigidBody playerBody;
    // playerBody.velocity = glm::vec3(1,1,0) * 2.f;
    playerBody.restitutionCoef = 1.0f;
    playerBody.mass = 3.f;
    playerBody.type = RigidBody::BodyTypeEnum::KINEMATIC;
    CollisionShape playerShape;
    playerShape.shapeType = SPHERE;
    playerShape.sphere.radius = 1.f;
    playerShape.layer = CollisionShape::PLAYER_LAYER;
    playerShape.mask = CollisionShape::ENV_LAYER;
    
    CustomBehavior playerBehavior;
    playerBehavior.update = [playerEntity, &ecs, groundCheckEntity,lastInputDir = glm::vec3(0,0,1)](float dt) mutable{
        auto& tr    = ecs.GetComponent<Transform>(playerEntity);
        auto& rb    = ecs.GetComponent<RigidBody>(playerEntity);
        auto& shape = ecs.GetComponent<CollisionShape>(playerEntity);
        auto& playerAnimation = ecs.GetComponent<AnimatedDrawable>(playerEntity);
        auto& groundCheck = ecs.GetComponent<CollisionShape>(groundCheckEntity);

        bool grounded = groundCheck.isAnythingColliding();

        glm::vec3 up      = -rb.gravityDirection;
        if(glm::length2(up) < 1e-6f) up = glm::vec3(0,1,0);
        glm::vec3 left;
        glm::vec3 forward;

        if(rb.gravityAnchor.x == 0 && rb.gravityAnchor.y == 0 && rb.gravityAnchor.z == 0){
            left    = glm::normalize(glm::cross(Camera::getInstance().camera_position - Camera::getInstance().camera_target,    glm::vec3(0,1,0)));
            forward = glm::normalize(glm::cross(left,  up));
        } else {
            left    = glm::normalize(glm::cross(up,    glm::vec3(0,1,0)));
            if(fabs(up.y)>0.999f)    forward = left;
            else     forward = glm::normalize(glm::cross(left,  up));
        }

        auto actions = InputManager::getInstance().getActions();
        glm::vec3 inputDir(0.0f);
        if(actions[InputManager::ActionEnum::ACTION_FORWARD ].pressed)  inputDir +=  forward;
        if(actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)  inputDir -=  forward;
        if(actions[InputManager::ActionEnum::ACTION_LEFT    ].pressed)  inputDir +=  left;
        if(actions[InputManager::ActionEnum::ACTION_RIGHT   ].pressed)  inputDir -=  left;
        if(glm::length2(inputDir) > 1e-6f) {
            inputDir = glm::normalize(inputDir);lastInputDir = inputDir;
            playerAnimation.playing = true;
        } else {
            playerAnimation.playing = false;
        }
        const float speed = 10.0f;
        glm::vec3 horizontalVel = inputDir * speed;


        float verticalSpeed = glm::dot(rb.velocity, rb.gravityDirection);
        const float jumpStrength = 8.0f;
        if(actions[InputManager::ActionEnum::ACTION_JUMP].pressed && grounded) {
            verticalSpeed = -jumpStrength;
        }else if(shape.isAnythingColliding()){
            verticalSpeed = 0.f;
        }else{
            verticalSpeed += 9.81f * dt;
        }
        
        rb.velocity = horizontalVel + rb.gravityDirection * verticalSpeed;
        tr.translate(rb.velocity * dt);

        
        glm::vec3 newUp = -glm::normalize(rb.gravityDirection);
        if (glm::length2(newUp) < 1e-6f) newUp = glm::vec3(0,1,0);
        glm::quat qAlign = glm::rotation(glm::vec3(0,1,0), newUp);
        glm::quat qYaw = glm::quat();
        glm::vec3 forwardLocal = qAlign * glm::vec3(0,0,1);
        qYaw = glm::rotation(forwardLocal, lastInputDir);

        glm::quat qFinal = glm::normalize(qYaw * qAlign);
        ecs.GetComponent<Transform>(playerEntity).setLocalRotation(qFinal);
    };


    // Drawable lowerRes = Render::generatePlane(1, 2);
    // auto lowerResEntity = ecs.CreateEntity();
    // ecs.AddComponent<Drawable>(lowerResEntity, lowerRes);
    // playerDraw.lodLower = &ecs.GetComponent<Drawable>(lowerResEntity);
    // playerDraw.switchDistance = 15;

    ecs.AddComponent(playerEntity, playerBehavior);
    ecs.AddComponent(playerEntity, playerBody);
    ecs.AddComponent(playerEntity, playerShape);




    std::unique_ptr<SpatialNode> playerNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(playerEntity));
    std::unique_ptr<SpatialNode> rayNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(groundCheckEntity));

    playerNode->AddChild(std::move(rayNode));
    parent.AddChild(std::move(playerNode));

    return playerEntity;
}

Entity generateWall(ecsManager &ecs, SpatialNode *parent){
    auto wallEntity = ecs.CreateEntity();
    Material wallMat;
    wallMat.albedoTex = &Texture::loadTexture("../assets/images/wall/blockPieceTex.png");
    wallMat.albedoTex->visible = true;
    // wallMat.normalTex = &Texture::loadTexture("../assets/images/wall/.png");
    wallMat.normalTex->visible = false;
    // wallMat.metallicTex = &Texture::loadTexture("../assets/images/wall/.png");
    wallMat.metallicTex->visible = false;
    // wallMat.roughnessTex = &Texture::loadTexture("../assets/images/PBR/oldMetal/Roughness.png");
    wallMat.aoTex = &Texture::loadTexture("../assets/images/wall/blockTex_Occ1.png");
    wallMat.aoTex->visible = true;
    //albedoTex, *normalTex, *metallicTex, *roughnessTex, *aoTex
    Drawable wallDrawable = Render::generatePlane(10, 2);
    // Render::loadSimpleMesh("../assets/meshes", "/Props/crate.glb", crateDrawable, crateMat);
    
    CollisionShape wallShape;
    wallShape.shapeType = PLANE;
    wallShape.plane.normal = glm::vec3(0,1,0);
    // wallShape.oobb.halfExtents = vec3(1.f, 1.f, 1.f);
    RigidBody wallBody;
    wallBody.type = RigidBody::STATIC;
    
    Transform wallTransform;
    ecs.AddComponent(wallEntity, wallTransform);
    ecs.AddComponent(wallEntity, wallShape);
    ecs.AddComponent(wallEntity, wallBody);
    ecs.AddComponent(wallEntity, wallDrawable);
    ecs.AddComponent(wallEntity, wallMat);

    std::unique_ptr<SpatialNode> wallNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(wallEntity));
    parent->AddChild(std::move(wallNode));

    return wallEntity;
}


Entity generateSingleTunnel(ecsManager &ecs, SpatialNode &parent, Entity &interactionEntity){
    Entity tunnel = ecs.CreateEntity();
    Transform tunnelTransform;
    Drawable tunnelDrawable;
    Material tunnelMaterial;
    RigidBody tunnelBody;
    CollisionShape tunnelShape;

    Render::loadSimpleMesh("../assets/meshes/Props", "/pipe.glb", tunnelDrawable, tunnelMaterial);
    // tunnelDrawable = Render::generateCube(1, 3);
    tunnelMaterial.albedoTex->visible = false;
    tunnelMaterial.albedo = glm::vec3(0.3, 1, 0.2);
    tunnelBody.type = RigidBody::STATIC;
    tunnelShape.shapeType = OOBB;
    tunnelShape.oobb.halfExtents = {1.1, 1.9,1.1};
    
    ecs.AddComponent(tunnel, tunnelTransform);
    ecs.AddComponent(tunnel, tunnelDrawable);
    ecs.AddComponent(tunnel, tunnelMaterial);
    ecs.AddComponent(tunnel, tunnelBody);
    ecs.AddComponent(tunnel, tunnelShape);


    interactionEntity = ecs.CreateEntity();
    Transform interactionTransform;
    CollisionShape interactionShape;

    interactionTransform.translate({0,3,0});
    interactionShape.shapeType = SPHERE;
    interactionShape.sphere.radius = 0.75f;
    interactionShape.layer = 0;
    interactionShape.mask = CollisionShape::PLAYER_LAYER;

    ecs.AddComponent(interactionEntity, interactionTransform);
    ecs.AddComponent(interactionEntity, interactionShape);

    std::unique_ptr<SpatialNode> tunnelNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(tunnel));
    std::unique_ptr<SpatialNode> interactionNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(interactionEntity));

    tunnelNode->AddChild(std::move(interactionNode));
    parent.AddChild(std::move(tunnelNode));
    return tunnel;
}
void generateTunnels(ecsManager &ecs, SpatialNode &parent, Entity &playerEntity, Entity &tunnelA, Entity &tunnelB){
    Entity interactionA, interactionB;
    tunnelA = generateSingleTunnel(ecs, parent, interactionA);
    tunnelB = generateSingleTunnel(ecs, parent, interactionB);

    CustomBehavior behaviorA;
    behaviorA.update = [interactionA, interactionB, playerEntity, &ecs](float delta) {
        auto actions = InputManager::getInstance().getActions();
        
        auto &collisionA = ecs.GetComponent<CollisionShape>(interactionA);
        if(collisionA.isAnythingColliding() && actions[InputManager::ACTION_INTERACT].clicked){
            Transform &playerTransform = ecs.GetComponent<Transform>(playerEntity);
            Transform &interactionBTransform = ecs.GetComponent<Transform>(interactionB);
            
            playerTransform.translate(interactionBTransform.getGlobalPosition() - playerTransform.getGlobalPosition());
        }
    };
    ecs.AddComponent(tunnelA, behaviorA);


    CustomBehavior behaviorB;
    behaviorB.update = [interactionA, interactionB, playerEntity, &ecs](float delta) {
        auto actions = InputManager::getInstance().getActions();
        
        if(ecs.GetComponent<CollisionShape>(interactionB).isAnythingColliding() && actions[InputManager::ACTION_INTERACT].clicked){
            Transform &playerTransform = ecs.GetComponent<Transform>(playerEntity);
            Transform &interactionATransform = ecs.GetComponent<Transform>(interactionA);
            
            playerTransform.translate(interactionATransform.getGlobalPosition() - playerTransform.getGlobalPosition());
        }
    };
    ecs.AddComponent(tunnelB, behaviorB);
}

Entity generateLevel1(SpatialNode &root, ecsManager &ecs, Entity &playerEntity){
    Entity levelCameraEntity = ecs.CreateEntity();
    Transform cameraTransform;
    cameraTransform.translate({0, 5, -15});
    CameraComponent levelCamComp;
    // levelCamComp.needActivation = true;
    levelCamComp.direction = glm::vec3(0,0,-1);
    ecs.AddComponent(levelCameraEntity, cameraTransform);
    ecs.AddComponent(levelCameraEntity, levelCamComp);


    Entity level = ecs.CreateEntity();
    ecs.SetEntityName(level, "Level1");
    Transform levelTransform;
    CollisionShape levelShape;
    levelShape.shapeType = OOBB;
    levelShape.oobb.halfExtents = {5,5,5};
    levelShape.layer = 0;
    levelShape.mask = CollisionShape::PLAYER_LAYER;
    CustomBehavior levelBehavior;
    levelBehavior.update = [levelCameraEntity, playerEntity, level, &ecs](float delta){
        auto &camComp = ecs.GetComponent<CameraComponent>(levelCameraEntity); 
        if(!camComp.activated && ecs.GetComponent<CollisionShape>(level).isColliding(playerEntity)){
            camComp.needActivation = true;
            camComp.direction = glm::vec3(0,0,-1);
            ecs.GetComponent<RigidBody>(playerEntity).removeAnchor();
            ecs.GetComponent<RigidBody>(playerEntity).gravityDirection = glm::vec3(0,-1,0);
        } else if(camComp.activated && !ecs.GetComponent<CollisionShape>(level).isColliding(playerEntity)){
            camComp.needActivation = false;
        }
    };
    ecs.AddComponent(level, levelShape);
    ecs.AddComponent(level, levelBehavior);
    ecs.AddComponent(level, levelTransform);

    Entity light1 = createLightSource(ecs, {-195,-195,-200}, {1,1,1});
    ecs.SetEntityName(light1, "Light 1");

    std::unique_ptr<SpatialNode> levelNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(level));
    levelNode->AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(levelCameraEntity)));
    levelNode->AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(light1)));

    
    Entity wall1 = generateWall(ecs, levelNode.get());
    ecs.GetComponent<Transform>(wall1).rotate({180,0,0});
    ecs.GetComponent<Transform>(wall1).translate({0,10,0});
    Entity wall2 = generateWall(ecs, levelNode.get());
    ecs.GetComponent<Transform>(wall2).rotate({-90,0,0});
    ecs.GetComponent<Transform>(wall2).translate({0,5,5});
    Entity wall3 = generateWall(ecs, levelNode.get());
    ecs.GetComponent<Transform>(wall3).rotate({0,0,90});
    ecs.GetComponent<Transform>(wall3).translate({5,5,0});
    Entity wall4 = generateWall(ecs, levelNode.get());
    ecs.GetComponent<Transform>(wall4).rotate({0,0,-90});
    ecs.GetComponent<Transform>(wall4).translate({-5,5,0});
    Entity wall5 = generateWall(ecs, levelNode.get());



    // levelCamComp.needActivation = true;


    
    root.AddChild(std::move(levelNode));

    return level;
}

Entity loadMeshLayer(SpatialNode &parent, ecsManager &ecs, char* folderPath, char* fileName, int layer){
    auto res = ecs.CreateEntity();
    Transform layer1Transform;
    Drawable sphereDraw;
    auto sphereMaterial = Material();
    Render::loadSimpleMesh(folderPath, fileName, sphereDraw, sphereMaterial, layer);
    ecs.AddComponent(res, layer1Transform);
    ecs.AddComponent(res, sphereDraw);
    ecs.AddComponent(res, sphereMaterial);


    std::unique_ptr<SpatialNode> meshNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(res));
    parent.AddChild(std::move(meshNode));


    return res;
}

Entity generatePlanet1(SpatialNode &root, ecsManager &ecs, Entity &playerEntity, glm::vec3 planetCenter){
    auto planetEntity = generatePlanetBody(ecs, planetCenter, 23.f);
    ecs.SetEntityName(planetEntity, "Planet 1");
    auto planetGravity = generateGravityArea(ecs, glm::vec3(0.f), 30.f, playerEntity);


    auto drawingNodeEntity = ecs.CreateEntity();
    ecs.SetEntityName(drawingNodeEntity, "Planet 1 Mesh");
    Transform drawingNodetransform;
    drawingNodetransform.setScale(glm::vec3(0.017));
    ecs.AddComponent(drawingNodeEntity, drawingNodetransform);



    std::unique_ptr<SpatialNode> planetNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(planetEntity));
    std::unique_ptr<SpatialNode> planetGravityNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(planetGravity));
    std::unique_ptr<SpatialNode> drawingNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(drawingNodeEntity));


    Entity currentEntity;
    currentEntity = loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 0);
    ecs.GetComponent<Drawable>(currentEntity).hideOnCubemapRender = true;
    currentEntity = loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 2);
    ecs.GetComponent<Drawable>(currentEntity).hideOnCubemapRender = true;
    currentEntity = loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 4);
    ecs.GetComponent<Drawable>(currentEntity).hideOnCubemapRender = true;
    currentEntity = loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 8);
    ecs.GetComponent<Drawable>(currentEntity).hideOnCubemapRender = true;
    currentEntity = loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 10);
    ecs.GetComponent<Drawable>(currentEntity).hideOnCubemapRender = true;
    currentEntity = loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 6);
    ecs.GetComponent<Drawable>(currentEntity).hideOnCubemapRender = true;

    float lightDistance = 30.f;
    createLightSource(ecs, planetCenter, {1,1,1});
    createLightSource(ecs, planetCenter + glm::vec3(lightDistance,0,0), {1,0.5,0.25});
    createLightSource(ecs, planetCenter + glm::vec3(-lightDistance,0,0), {1,0.5,0.25});
    createLightSource(ecs, planetCenter + glm::vec3(0,0,lightDistance), {1,0.5,0.25});
    createLightSource(ecs, planetCenter + glm::vec3(0,0,-lightDistance), {1,0.5,0.25});
    createLightSource(ecs, planetCenter + glm::vec3(0,lightDistance,0), {1,0.5,0.25});
    createLightSource(ecs, planetCenter + glm::vec3(0,-lightDistance,0), {1,0.5,0.25});

    planetNode->AddChild(std::move(planetGravityNode));
    planetNode->AddChild(std::move(drawingNode));
    root.AddChild(std::move(planetNode));

    return planetEntity;
}

Entity generatePlanet2(SpatialNode &root, ecsManager &ecs, Entity &playerEntity, glm::vec3 planetCenter){
    auto planetEntity = generatePlanetBody(ecs, planetCenter, 10.f);
    ecs.SetEntityName(planetEntity, "Planet 2");
    auto planetGravity = generateGravityArea(ecs, glm::vec3(0.f), 20.f, playerEntity);


    auto drawingNodeEntity = ecs.CreateEntity();
    ecs.SetEntityName(drawingNodeEntity, "Planet 2 Mesh");
    Transform drawingNodetransform;
    drawingNodetransform.setScale(glm::vec3(0.008));
    ecs.AddComponent(drawingNodeEntity, drawingNodetransform);



    std::unique_ptr<SpatialNode> planetNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(planetEntity));
    std::unique_ptr<SpatialNode> planetGravityNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(planetGravity));
    std::unique_ptr<SpatialNode> drawingNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(drawingNodeEntity));


    loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 0);
    loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 2);
    loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 4);
    loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 8);
    loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 10);
    loadMeshLayer(*drawingNode.get(), ecs, "../assets/meshes/Props", "/planet_1.glb", 6);

    
    float lightDistance = 20.f;
    createLightSource(ecs, planetCenter, {1,1,1});
    createLightSource(ecs, planetCenter + glm::vec3(lightDistance,0,0), {1,1,1});
    createLightSource(ecs, planetCenter + glm::vec3(-lightDistance,0,0), {1,1,1});
    createLightSource(ecs, planetCenter + glm::vec3(0,0,lightDistance), {1,1,1});
    createLightSource(ecs, planetCenter + glm::vec3(0,0,-lightDistance), {1,1,1});


    planetNode->AddChild(std::move(planetGravityNode));
    planetNode->AddChild(std::move(drawingNode));
    root.AddChild(std::move(planetNode));

    return planetEntity;
}

void initScene(SpatialNode &root, ecsManager &ecs){
    Program::programs.push_back(std::make_unique<PBR>());    

    Entity playerEntity = generatePlayer(ecs, root);


    Entity levelEntity = generateLevel1(root, ecs, playerEntity);
    ecs.GetComponent<Transform>(levelEntity).translate({-200,-200,-200});

    Entity tunnelA, tunnelB,tunnelC, tunnelD;
    generateTunnels(ecs, root, playerEntity, tunnelA, tunnelB);
    generateTunnels(ecs, root, playerEntity, tunnelC, tunnelD);


    // ecs.GetComponent<Transform>(tunnelA).translate({0,12,6});
    // ecs.GetComponent<Transform>(tunnelA).rotate({0,-39,52});
    ecs.GetComponent<Transform>(tunnelA).translate({14-1e-3f,22,14-1e-3f});
    ecs.GetComponent<Transform>(tunnelA).changeScale(glm::vec3(1.f,1.f,1.f));
    ecs.GetComponent<Transform>(tunnelB).translate({-198,-200,-200});

    ecs.GetComponent<Transform>(tunnelC).translate({14+21,0,14});
    ecs.GetComponent<Transform>(tunnelC).rotate({0,0,-90});
    ecs.GetComponent<Transform>(tunnelD).translate({80+10,80,80});
    ecs.GetComponent<Transform>(tunnelD).rotate({0,0,-90});



    glm::vec3 planetCenter = {14, 0, 14};
    generatePlanet1(root, ecs, playerEntity, planetCenter);

    glm::vec3 planetCenter2 = {80, 80, 80};
    generatePlanet2(root, ecs, playerEntity, planetCenter2);
    // auto planetEntity = generatePlanetBody(ecs, planetCenter, 20.f);
    // auto planetGravity = generateGravityArea(ecs, glm::vec3(0.f), 60.f, playerEntity);

    /////////////////////////////// collision debug
    // auto otherEntity = ecs.CreateEntity();
    // Transform otherTransform;
    // otherTransform.translate(glm::vec3(5,10,0));
    // Light lightSource;
    // lightSource.color = glm::vec3(1,1,1);
    // CollisionShape otherCollision;
    // otherCollision.shapeType = SPHERE;
    // otherCollision.sphere.radius = 1.2;
    // ecs.AddComponent(otherEntity, otherTransform);
    // ecs.AddComponent(otherEntity, lightSource);
    // ecs.AddComponent(otherEntity, otherCollision);



    // auto crateEntity = generateCrate(ecs, {0,20, 0});
    // ecs.SetEntityName(crateEntity, "crate1");
    // auto crateEntity2 = generateCrate(ecs, {0,35, 0});
    // ecs.SetEntityName(crateEntity2, "crate2");

    // root.AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(crateEntity)));
    // root.AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(crateEntity2)));
    

    
    ///////////////////////////// camera
    auto cameraEntity = ecs.CreateEntity();
    ecs.SetEntityName(cameraEntity, "Camera player default");
    CustomBehavior cameraUpdate;
    Transform cameraTransform;
    cameraTransform.translate({0,100,0});
    CameraComponent cameraComponent;
    cameraComponent.needActivation = true;
    cameraUpdate.update = [playerEntity, cameraEntity, &ecs](float deltaTime){
        RigidBody& targetBody = ecs.GetComponent<RigidBody>(playerEntity);
        Transform &targetTransform = ecs.GetComponent<Transform>(playerEntity);
        glm::vec3 targetPosition = targetTransform.getGlobalPosition();

        Transform &camTransform = ecs.GetComponent<Transform>(cameraEntity);
        CameraComponent &camComp = ecs.GetComponent<CameraComponent>(cameraEntity);

        glm::vec3 up = glm::length2(targetBody.gravityDirection) > 1e-6f
                   ? -glm::normalize(targetBody.gravityDirection)
                   : glm::vec3(0,1,0);

        glm::vec3 diffTarget = camTransform.getGlobalPosition() - targetTransform.getGlobalPosition();
        glm::vec3 right   = glm::normalize(glm::cross(up, glm::normalize(diffTarget)));
        glm::vec3 forward = glm::normalize(glm::cross(right, up));


        const float behindDistance = 10.0f;
        const float aboveDistance  = 20.0f;
        const float predictionDistance = 15.f;
        const float minSpeedRequierd = 0.5f;


        glm::vec3 goalPos;
        
        glm::vec3 goalTarget;
        if(glm::length(targetBody.velocity) >= minSpeedRequierd){
            goalTarget = targetPosition + glm::normalize(targetBody.velocity) * predictionDistance;
            goalPos = targetPosition + up * aboveDistance + glm::normalize(targetBody.velocity) * behindDistance;
        } else {
            goalTarget = targetPosition;
            goalPos = targetPosition + up * (aboveDistance + behindDistance);
        }
        glm::vec3 goalDirection = glm::normalize(goalPos - goalTarget);
        
        
        glm::vec3 newPos = glm::mix(camTransform.getGlobalPosition(), goalPos, deltaTime);

        glm::vec3 newDirection = glm::mix(camComp.direction, goalDirection, deltaTime);

        camTransform.setLocalPosition(newPos);
        // ecs.GetComponent<CameraComponent>(cameraEntity).direction = glm::normalize(camTransform.getGlobalPosition() - targetTransform.getGlobalPosition());
        camComp.direction = newDirection;
        // ecs.GetComponent<CameraComponent>(cameraEntity).up = up;
    };
    ecs.AddComponent(cameraEntity, cameraUpdate);
    ecs.AddComponent(cameraEntity, cameraTransform);
    ecs.AddComponent(cameraEntity, cameraComponent);


    auto rootEntity = ecs.CreateEntity();
    Transform rootTransform;
    ecs.AddComponent(rootEntity, rootTransform);
    root.transform = &ecs.GetComponent<Transform>(rootEntity);
    
    std::unique_ptr<SpatialNode> playerCameraNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(cameraEntity));
    // std::unique_ptr<SpatialNode> b1Node = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(b1Entity));
    // std::unique_ptr<SpatialNode> b2Node = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(b2Entity));
    // std::unique_ptr<SpatialNode> otherNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(otherEntity));

    root.AddChild(std::move(playerCameraNode));
    // root.AddChild(std::move(otherNode));
    // root.AddChild(std::move(b1Node));
    // root.AddChild(std::move(b2Node));
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
    ecs.AddComponent(rootEntity, continuousRotation);

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
    ecs.AddComponent(movingLight, oscilatingLight);











    auto animationEntity = ecs.CreateEntity();
    ecs.SetEntityName(animationEntity, "Animation");
    Transform animationTransform;
    animationTransform.translate({0,0,0});
    // AnimatedDrawable animationDraw = AnimatedPBRrender::loadMesh("../assets/meshes/Mario-walk.glb");
    // AnimatedDrawable animationDraw = AnimatedPBRrender::loadMesh("../assets/meshes/Walking.glb");
    AnimatedDrawable animationDraw;
    Material animationMaterial;
    animationMaterial.albedo = {0.5f,0.5f,0.5f};
    AnimatedPBRrender::loadMesh("../assets/meshes", "/Walking.glb", animationDraw, animationMaterial);
    ecs.AddComponent(animationEntity, animationTransform);
    ecs.AddComponent(animationEntity, animationDraw);
    ecs.AddComponent(animationEntity, animationMaterial);

    root.AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(animationEntity)));
}



void physicScene(SpatialNode &root, ecsManager &ecs){
    auto rootEntity = ecs.CreateEntity();
    Transform rootTransform;
    ecs.AddComponent(rootEntity, rootTransform);
    root.transform = &ecs.GetComponent<Transform>(rootEntity);

    auto cameraEntity = ecs.CreateEntity();
    ecs.SetEntityName(cameraEntity, "Camera player default");
    Transform cameraTransform;
    cameraTransform.translate({0,10,-50});
    CameraComponent cameraComponent;
    cameraComponent.needActivation = true;
    ecs.AddComponent(cameraEntity, cameraTransform);
    ecs.AddComponent(cameraEntity, cameraComponent);
    root.AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(cameraEntity)));

    auto crateEntity = generateCrate(ecs, {0,20, 0});
    ecs.SetEntityName(crateEntity, "crate1");
    ecs.GetComponent<CollisionShape>(crateEntity).shapeType = OOBB;
    ecs.GetComponent<CollisionShape>(crateEntity).oobb.halfExtents = glm::vec3(1);
    auto crateEntity2 = generateCrate(ecs, {0,10, 0});
    ecs.SetEntityName(crateEntity2, "crate2");
    ecs.GetComponent<CollisionShape>(crateEntity2).shapeType = OOBB;
    ecs.GetComponent<CollisionShape>(crateEntity2).oobb.halfExtents = glm::vec3(1,1,1);
    

    root.AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(crateEntity)));
    root.AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(crateEntity2)));


    Entity groundE = ecs.CreateEntity();
    Transform groundTransform;
    Drawable groundDraw = Render::generatePlane(100.f, 2);
    Material groundMat;
    CollisionShape groundShape;
    // groundShape.shapeType = PLANE;
    // groundShape.plane.normal = glm::vec3(0,1,0);
    groundShape.shapeType = OOBB;
    groundShape.oobb.halfExtents = {20, 1, 20};
    RigidBody groundBody;
    groundBody.type = RigidBody::STATIC;

    ecs.AddComponent(groundE, groundTransform);
    ecs.AddComponent(groundE, groundBody);
    ecs.AddComponent(groundE, groundShape);
    ecs.AddComponent(groundE, groundDraw);
    ecs.AddComponent(groundE, groundMat);
    root.AddChild(std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(groundE)));

    Entity eggSpawner = ecs.CreateEntity();
    CustomBehavior eggSpawnerBehavior;
    eggSpawnerBehavior.update = [&ecs, &root](float delta){
        auto actions = InputManager::getInstance().getActions();
        if(actions[InputManager::ActionEnum::ACTION_JUMP ].clicked){
            auto egg = generateEgg(ecs, &root, {7, 10, 0});
        }
    };
    ecs.AddComponent(eggSpawner, eggSpawnerBehavior);

    auto plane = generateWall(ecs, &root);
    CollisionShape &planeShape = ecs.GetComponent<CollisionShape>(plane);
    planeShape.shapeType = PLANE;
    planeShape.plane.normal = glm::vec3(0,1,0);
    planeShape.plane.left = glm::vec3(1,0,0);
    // planeShape.oobb.halfExtents = glm::vec3(5,1,5);
    ecs.GetComponent<Transform>(plane).rotate({-30,0,0});
    ecs.GetComponent<Transform>(plane).translate({10,5,0});
}