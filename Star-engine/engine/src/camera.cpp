#include <engine/include/camera.hpp>

Camera& Camera::getInstance() {
    static Camera instance;
    return instance;
}

glm::mat4 Camera::getVP(){    
    glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_target, camera_up);

    glm::mat4 projection = glm::perspective(glm::radians(45.f), 800.0f / 600.0f, 0.1f, 100.0f);

    glm::mat4 vp = projection * view;

    return vp;
}

glm::vec3 rotateY(glm::vec3 in, float angle){
    glm::vec3 out(in);
    out.x = cos(angle) * in.x - sin(angle) * in.z;
    out.z = sin(angle) * in.x + cos(angle) * in.z;

    return out;
}

glm::vec3 rotateX(glm::vec3 in, float angle){
    glm::vec3 out(in);
    out.y = cos(angle) * in.y - sin(angle) * in.z;
    out.z = sin(angle) * in.y + cos(angle) * in.z;

    return out;
}

void Camera::update(float deltaTime){
    auto actions = InputManager::getInstance().getActions();     
    if(actions[InputManager::ACTION_INCREASE_ROTATION_SPEED].pressed){
        rotationSpeed += 0.01f;
    }
    if(actions[InputManager::ACTION_DECREASE_ROTATION_SPEED].pressed){
        rotationSpeed -= 0.01f;
    }
    // if(actions[InputManager::ACTION_SWITCH_ROTATION].clicked){
    //     orbiting = !orbiting;
    //     if(!orbiting){
    //         camera_position = glm::vec3(-orbitRange/2.f, 0, -orbitRange);
    //         camera_target = glm::vec3(0.0f, 0.0f, 1.0f);
    //     }
    // }

    float cameraDelta = cameraSpeed * deltaTime;
    
    if (actions[InputManager::ActionEnum::ACTION_FORWARD].pressed)
        camera_position += cameraDelta * camera_target;
    if (actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)
        camera_position -= cameraDelta * camera_target;

    glm::vec3 left = glm::cross(camera_up, camera_target);
    if (actions[InputManager::ActionEnum::ACTION_LEFT].pressed)
        camera_position += cameraDelta * left;
    if (actions[InputManager::ActionEnum::ACTION_RIGHT].pressed)
        camera_position -= cameraDelta * left;
    if (actions[InputManager::ActionEnum::ACTION_UP].pressed){
        camera_position += glm::vec3(0, 0.1, 0);
    }
    if (actions[InputManager::ActionEnum::ACTION_DOWN].pressed)
        camera_position -= glm::vec3(0, 0.1, 0);

    if (actions[InputManager::ActionEnum::ACTION_LOOK_UP].pressed)
        hAngle += deltaTime;
    if (actions[InputManager::ActionEnum::ACTION_LOOK_DOWN].pressed)
        hAngle -= deltaTime;
    if (actions[InputManager::ActionEnum::ACTION_LOOK_RIGHT].pressed)
        vAngle += deltaTime;
    if (actions[InputManager::ActionEnum::ACTION_LOOK_LEFT].pressed)
        vAngle -= deltaTime;
    
    camera_target = glm::vec3(0,0,-1);
    camera_target = rotateX(camera_target, hAngle);
    camera_target = rotateY(camera_target, vAngle);
}