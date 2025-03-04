#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <TP1/include/input.hpp>

namespace Camera{
    glm::vec3 camera_position   = glm::vec3(0.0f, 0.0f,  3.0f);
    glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

    float hAngle = 0;
    float vAngle = 0;
    float mSpeed = 0.05f;

    float cameraSpeed = 2.5f;


    bool rotating = false;
    float rotationSpeed = 2.5f;


    glm::mat4 getMVP(){
        glm::mat4 model( 1.0f );
            
        
        glm::mat4 view;
        if(rotating){
            view = glm::lookAt(camera_position, glm::vec3(0), camera_up);
        } else {
            view = glm::lookAt(camera_position, camera_position + camera_target, camera_up);
        }

        glm::mat4 projection = glm::perspective(glm::radians(45.f), 800.0f / 600.0f, 0.1f, 100.0f);

        glm::mat4 mvp = projection * view * model;

        return mvp;
    }

    void update(float deltaTime){        
        if(Input::actions[Input::ACTION_INCREASE_ROTATION_SPEED].pressed){
            rotationSpeed += 0.1f;
        }
        if(Input::actions[Input::ACTION_DECREASE_ROTATION_SPEED].pressed){
            rotationSpeed -= 0.1f;
        }
        if(Input::actions[Input::ACTION_SWITCH_ROTATION].clicked){
            rotating = !rotating;
            if(rotating)
                camera_position = glm::vec3(5.f);
            else{
                camera_position = glm::vec3(0.f);
                camera_target = glm::vec3(0.0f, 0.0f, 1.0f);
            }
        }

        float cameraDelta = cameraSpeed * deltaTime;

        if (Input::actions[Input::ActionEnum::ACTION_FORWARD].pressed)
            camera_position += cameraDelta * camera_target;
        if (Input::actions[Input::ActionEnum::ACTION_BACKWARD].pressed)
            camera_position -= cameraDelta * camera_target;
            
        if(rotating){
            camera_target = -glm::normalize(camera_position);
            glm::vec3 left = glm::cross(camera_up, camera_target);
            camera_position -= left * rotationSpeed * deltaTime;
        } else {           
            glm::vec3 left = glm::cross(camera_up, camera_target);
            if (Input::actions[Input::ActionEnum::ACTION_LEFT].pressed)
                camera_position += cameraDelta * left;
            if (Input::actions[Input::ActionEnum::ACTION_RIGHT].pressed)
                camera_position -= cameraDelta * left;
            if (Input::actions[Input::ActionEnum::ACTION_UP].pressed){
                camera_position += glm::vec3(0, 0.1, 0);
            }
            if (Input::actions[Input::ActionEnum::ACTION_DOWN].pressed)
                camera_position -= glm::vec3(0, 0.1, 0);
        }
    }

}

