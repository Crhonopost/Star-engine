#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/camera.hpp>




void CameraSystem::update(){
    for (const auto& entity : mEntities) {
        auto& cam = ecs.GetComponent<CameraComponent>(entity);
        if(cam.needActivation && !cam.activated){
            cam.activated = true;
            cams.push(entity);
        }
    }

    while(!ecs.GetComponent<CameraComponent>(cams.top()).needActivation){
        cams.pop();
    }

    if(cams.size() == 0){
        std::cerr << "no valid camera detected!!";
        return;
    } else {
        auto camEntity = cams.top();
        auto& cam = ecs.GetComponent<CameraComponent>(camEntity);        
        auto& transform = ecs.GetComponent<Transform>(camEntity);

        glm::vec3 newPos = transform.getGlobalPosition();
        
        Camera::getInstance().camera_position = newPos;
        
        // glm::vec3 forward = glm::mat3(transform.getModelMatrix()) * glm::vec3(0,0,1);
        
        Camera::getInstance().camera_target = newPos - cam.direction;// Camera::getInstance().camera_position + glm::normalize(forward);
        Camera::getInstance().camera_up = cam.up;
    }
}


void CustomSystem::update(float deltaTime){
    for(auto const& entity: mEntities){
        auto& behavior = ecs.GetComponent<CustomBehavior>(entity);

        behavior.update(deltaTime);
    }
}