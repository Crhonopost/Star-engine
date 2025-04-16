#include <engine/include/input.hpp>

// Définition du singleton
InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

// Constructeur privé qui initialise les touches
InputManager::InputManager() {
    actions[KEY_ESCAPE] = { GLFW_KEY_ESCAPE };
    actions[KEY_PLUS] = { GLFW_KEY_KP_ADD };
    actions[KEY_MINUS] = { GLFW_KEY_KP_SUBTRACT };

    actions[ACTION_FORWARD] = { GLFW_KEY_W };
    actions[ACTION_BACKWARD] = { GLFW_KEY_S };
    actions[ACTION_LEFT] = { GLFW_KEY_A };
    actions[ACTION_RIGHT] = { GLFW_KEY_D };

    actions[ACTION_UP] = { GLFW_KEY_E };
    actions[ACTION_DOWN] = { GLFW_KEY_Q };

    actions[ACTION_SWITCH_SCENE_ROTATION] = { GLFW_KEY_R };
    actions[ACTION_SWITCH_ROTATION] = { GLFW_KEY_C };
    actions[ACTION_INCREASE_ROTATION_SPEED] = { GLFW_KEY_UP };
    actions[ACTION_DECREASE_ROTATION_SPEED] = { GLFW_KEY_DOWN };

    actions[ACTION_LOOK_UP] = { GLFW_KEY_UP };
    actions[ACTION_LOOK_DOWN] = { GLFW_KEY_DOWN };
    actions[ACTION_LOOK_LEFT] = { GLFW_KEY_LEFT };
    actions[ACTION_LOOK_RIGHT] = { GLFW_KEY_RIGHT };

    actions[ACTION_LOCK_POSITION] = { GLFW_KEY_R};

    actions[ACTION_JUMP] = { GLFW_KEY_SPACE};

    actions[EDITOR_SWITCH_MODE] = {GLFW_KEY_ENTER};
}

// Traitement des inputs
void InputManager::processInput(GLFWwindow* window) {
    for (auto& [action, state] : actions) {
        if (glfwGetKey(window, state.glfw_key) == GLFW_PRESS) {
            state.pressed = true;
        } else {
            if (state.clicked) {
                state.clicked = false;
            }
            if (state.pressed) {
                state.clicked = true;
            }
            state.pressed = false;
        }
    }
}
