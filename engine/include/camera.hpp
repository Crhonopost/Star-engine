#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/include/input.hpp>

class Camera {
public:
    static Camera& getInstance(); // Accès unique à l'instance

    glm::mat4 getV();
    glm::mat4 getP();
    glm::vec3 getPosition();
    void updateInput(float deltaTime);

    glm::vec3 camera_position   = glm::vec3(0.0f, 0.0f,  0.0f);    
    glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

    float hAngle = 0;
    float vAngle = 0;
    float lastX, lastY;
    
    float mSpeed = 0.1f;
    float cameraSpeed = 10.5f;
    float rotationSpeed = .5f;
    
    bool locked = false;

    static bool editor;

    static glm::quat lookAtQuat(glm::vec3 direction, glm::vec3 up = glm::vec3(0, 1, 0)) {
        direction = glm::normalize(direction);
        glm::vec3 right = glm::normalize(glm::cross(up, direction));
        glm::vec3 correctedUp = glm::cross(direction, right);
    
        glm::mat3 rotationMatrix(right, correctedUp, direction);
        return glm::quat_cast(rotationMatrix);
    }

    static glm::vec3 lookAt(glm::vec3 direction, glm::vec3 up = {0, 1, 0}) {
        if (glm::length(direction) < 0.0001f)
            return glm::vec3(0);

        glm::quat rot = lookAtQuat(glm::normalize(direction), up);
        return glm::degrees(glm::eulerAngles(rot)); 
    }
    
private:
    Camera() = default; // Constructeur privé
    ~Camera() = default;
    
    Camera(const Camera&) = delete; // Empêcher la copie
    Camera& operator=(const Camera&) = delete;
};
