#include <GLFW/glfw3.h>
#include <map>

#pragma once;

namespace Input{
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
        ACTION_SWITCH_ROTATION,
        ACTION_INCREASE_ROTATION_SPEED,
        ACTION_DECREASE_ROTATION_SPEED
    };
    
    // TODO: HOW to prevent ActionState to be modified from outside of processInput
    struct ActionState{
        int glfw_key;
        bool clicked = false;
        bool pressed = false;
    };

    std::map<ActionEnum, ActionState> actions;
    bool instanciated = false;

    struct InputManager{
        private:
        static void init(){
            instanciated = true;

            actions[KEY_ESCAPE].glfw_key = GLFW_KEY_ESCAPE;
            actions[KEY_PLUS].glfw_key = GLFW_KEY_KP_ADD;
            actions[KEY_MINUS].glfw_key = GLFW_KEY_KP_SUBTRACT;
            

            
            actions[ACTION_FORWARD].glfw_key = GLFW_KEY_W;
            actions[ACTION_BACKWARD].glfw_key = GLFW_KEY_S;
            actions[ACTION_LEFT].glfw_key = GLFW_KEY_A;
            actions[ACTION_RIGHT].glfw_key = GLFW_KEY_D;

            actions[ACTION_UP].glfw_key = GLFW_KEY_E;
            actions[ACTION_DOWN].glfw_key = GLFW_KEY_Q;

            actions[ACTION_SWITCH_ROTATION].glfw_key = GLFW_KEY_C;
            actions[ACTION_INCREASE_ROTATION_SPEED].glfw_key = GLFW_KEY_UP;
            actions[ACTION_DECREASE_ROTATION_SPEED].glfw_key = GLFW_KEY_DOWN;
        }
        public:
        static void processInput(GLFWwindow *window)
        {
            if(!instanciated) init();

            std::map<ActionEnum, ActionState>::iterator it = actions.begin();
            while(it != actions.end()){
                ActionState &state = it->second;
                
                if(glfwGetKey(window, state.glfw_key) == GLFW_PRESS){
                    state.pressed = true;
                } else{
                    if(state.clicked){
                        state.clicked = false;
                    }
                    if(state.pressed){
                        state.clicked = true;
                    }
                    state.pressed = false;
                }
                ++it; 
            }
        }
    };
}
