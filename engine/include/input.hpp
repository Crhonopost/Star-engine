#pragma once

#include <GLFW/glfw3.h>
#include <map>

class InputManager {
public:
    enum ActionEnum {
        KEY_ESCAPE,
        KEY_PLUS,
        KEY_MINUS,

        ACTION_FORWARD,
        ACTION_BACKWARD,
        ACTION_LEFT,
        ACTION_RIGHT,
        ACTION_UP,
        ACTION_DOWN,
        ACTION_SWITCH_SCENE_ROTATION,
        ACTION_SWITCH_ROTATION,
        ACTION_INCREASE_ROTATION_SPEED,
        ACTION_DECREASE_ROTATION_SPEED,
        ACTION_LOOK_UP,
        ACTION_LOOK_DOWN,
        ACTION_LOOK_LEFT,
        ACTION_LOOK_RIGHT,
        ACTION_LOCK_POSITION,
        ACTION_JUMP,
        ACTION_INTERACT,

        EDITOR_SWITCH_MODE
    };

    struct ActionState {
        int glfw_key;
        bool clicked = false;
        bool pressed = false;
    };

    static InputManager& getInstance(); // Accès unique au singleton
    void processInput(GLFWwindow* window);
    
    const std::map<ActionEnum, ActionState>& getActions() const { return actions; } // Accès en lecture

private:
    std::map<ActionEnum, ActionState> actions;

    InputManager(); // Constructeur privé
    ~InputManager() = default;

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
};
