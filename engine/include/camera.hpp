#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/include/input.hpp>

class Camera {
public:
    static Camera& getInstance(); // Accès unique à l'instance

    glm::mat4 getVP();
    void updateInput(float deltaTime);

    glm::vec3 camera_position   = glm::vec3(0.0f, 0.0f,  3.0f);    
    glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

    float hAngle = 0;
    float vAngle = 0;
    float lastX, lastY;
    
    float mSpeed = 0.1f;
    float cameraSpeed = 2.5f;
    float rotationSpeed = .5f;
    
    bool locked = false;

private:
    Camera() = default; // Constructeur privé
    ~Camera() = default;
    
    Camera(const Camera&) = delete; // Empêcher la copie
    Camera& operator=(const Camera&) = delete;
};
