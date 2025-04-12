// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <iostream>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

// personnals
#include <engine/include/input.hpp>
#include <engine/include/program.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/ecs/implementations/components.hpp>
#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/camera.hpp>
#include <engine/include/spatial.hpp>
#include <engine/include/stbi.h>
#include <engine/include/scene.hpp>



void userInteractions(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

//rotation
float angle = 0.;
float zoom = 1.;
/*******************************************************************************/

// SceneGraph scene;
ecsManager ecs;


int main( void )
{
    {

        GLenum error = glGetError();
    
        // Initialise GLFW
        if( !glfwInit() )
        {
            fprintf( stderr, "Failed to initialize GLFW\n" );
            getchar();
            return -1;
        }
    
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
        window = glfwCreateWindow( 1024, 768, "engine - GLFW", NULL, NULL);
        if( window == NULL ){
            fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
            getchar();
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(window);
    
        glewExperimental = true; // Needed for core profile
        if (glewInit() != GLEW_OK) {
            fprintf(stderr, "Failed to initialize GLEW\n");
            getchar();
            glfwTerminate();
            return -1;
        }
    
        glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
        //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
        glfwPollEvents();
        glfwSetCursorPos(window, 1024/2, 768/2);
    
        glClearColor(0.1f, 0.1f, 0.2f, 0.0f);
    
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    
        //glEnable(GL_CULL_FACE);
    
        // scene.init();
    
    
        // For speed computation
        int nbFrames = 0;
    
        ecs.Init();
        auto actions = InputManager::getInstance().getActions();
        
        ecs.RegisterComponent<Transform>();
        ecs.RegisterComponent<Drawable>();
        ecs.RegisterComponent<CustomBehavior>();
        ecs.RegisterComponent<RigidBody>();
        ecs.RegisterComponent<CollisionShape>();
    
        auto renderSystem = ecs.RegisterSystem<Render>();
        auto customSystem = ecs.RegisterSystem<CustomSystem>();
        auto physicSystem = ecs.RegisterSystem<PhysicSystem>();
        auto physicDebugSystem = ecs.RegisterSystem<PhysicDebugSystem>();
        physicDebugSystem->init();
        
        Signature renderSignature;
        renderSignature.set(ecs.GetComponentType<Transform>());
        renderSignature.set(ecs.GetComponentType<Drawable>());
        ecs.SetSystemSignature<Render>(renderSignature);
    
        Signature customSignature;
        customSignature.set(ecs.GetComponentType<CustomBehavior>());
        ecs.SetSystemSignature<CustomSystem>(customSignature);

        Signature physicSignature;
        physicSignature.set(ecs.GetComponentType<RigidBody>());
        physicSignature.set(ecs.GetComponentType<Transform>());        
        physicSignature.set(ecs.GetComponentType<CollisionShape>());        
        ecs.SetSystemSignature<PhysicSystem>(physicSignature);

        Signature physicDebugSignature;
        physicDebugSignature.set(ecs.GetComponentType<Transform>());        
        physicDebugSignature.set(ecs.GetComponentType<CollisionShape>());        
        ecs.SetSystemSignature<PhysicDebugSystem>(physicDebugSignature);



        SpatialNode root;
        initScene(root, ecs);
        
        
        do{
            // sunTransform.computeModelMatrix();
            // error = glGetError();
            // if(error != GL_NO_ERROR){
            //     std::cerr << "OpenGL error: " << error << std::endl;
            // // } 
            
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            // input
            InputManager::getInstance().processInput(window);
            userInteractions(window);
            actions = InputManager::getInstance().getActions();
            
            // Clear the screen
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            root.updateSelfAndChildTransform();

    
            customSystem->update(deltaTime);
            physicSystem->update(deltaTime);
            physicDebugSystem->update();
            renderSystem->update();
    
            
    
            // Swap buffers
            glfwSwapBuffers(window);
            glfwPollEvents();
    
        } // Check if the ESC key was pressed or the window was closed
        while( !actions[InputManager::ActionEnum::KEY_ESCAPE].clicked &&
               glfwWindowShouldClose(window) == 0 );
    
        // scene.clear();
    
        for(auto &prog: Program::programs){
            prog.clear();
        }
    }

    glfwTerminate();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void userInteractions(GLFWwindow *window)
{
    auto actions = InputManager::getInstance().getActions();
    if(actions[InputManager::KEY_ESCAPE].clicked){
        glfwSetWindowShouldClose(window, true);
    }
    
    // if(Input::actions[Input::ActionEnum::KEY_PLUS].pressed){
    //     scene.mountain.addVerticesQuantity(1);
    // }
    // if(Input::actions[Input::ActionEnum::KEY_MINUS].pressed){
    //     if(scene.mountain.getNumberOfVertices() > 2){
    //         scene.mountain.addVerticesQuantity(-1);
    //     }
    // }

    // Camera::getInstance().updateInput(deltaTime);   

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
