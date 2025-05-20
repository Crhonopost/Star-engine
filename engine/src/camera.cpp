#include <engine/include/camera.hpp>
#include <engine/include/geometryHelper.hpp>

bool Camera::editor = true;

Camera& Camera::getInstance() {
    static Camera instance;
    static Camera editorInstance;
    return editor ? editorInstance : instance;
}

glm::mat4 Camera::getV(){    
    return glm::lookAt(camera_position, camera_target, camera_up);
}

glm::mat4 Camera::getP(){
    return glm::perspective(glm::radians(45.f), view_width / view_height, 0.1f, 9999.0f);
}

glm::vec3 Camera::getPosition(){
    return camera_position;
}

void Camera::updateInput(float deltaTime){
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
    glm::vec3 forward = glm::normalize(camera_target - camera_position);
    
    if (actions[InputManager::ActionEnum::ACTION_FORWARD].pressed)
        camera_position += cameraDelta * forward;
    if (actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)
        camera_position -= cameraDelta * forward;

    glm::vec3 left = glm::cross(camera_up, forward);
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
    
    forward = glm::vec3(0,0,-1);
    forward = rotateX(forward, hAngle);
    forward = rotateY(forward, vAngle);

    camera_target = camera_position + forward;
}