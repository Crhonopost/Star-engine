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
#include <TP1/include/texture.hpp>
#include <TP1/include/input.hpp>
#include <TP1/include/scene.hpp>
#include <TP1/include/camera.hpp>


using namespace Camera;


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

Input::InputManager inputManager;
Scene scene;

int main( void )
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

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "TP1 - GLFW", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders( "shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl" );
    glUseProgram(programID);

    /*****************TODO***********************/
    GLuint mvpID = glGetUniformLocation(programID, "mvp");

    scene = Scene(programID);

    //Chargement du fichier de maillage
    // std::string filename("meshes/chair.off");
    // loadOFF(filename, indexed_vertices, indices, triangles );

    // Get a handle for our "LightPosition" uniform
    // GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    GLuint pixelSizeLocation = glGetUniformLocation(programID, "pixelSize");
    glUniform2f(pixelSizeLocation, 0.001, 0.001);
    GLuint timeLocation = glGetUniformLocation(programID, "time");
    do{
        // error = glGetError();
        // if(error != GL_NO_ERROR){
        //     std::cerr << "OpenGL error: " << error << std::endl;
        // }

        glUniform1f(timeLocation, (float) lastTime);


        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        inputManager.processInput(window);
        userInteractions(window);


        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);


        /*****************TODO***********************/
        glm::mat4 mvp = Camera::getMVP();

        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvp[0][0]);


        /****************************************/
        
        scene.render(programID);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        lastTime = glfwGetTime();

    } // Check if the ESC key was pressed or the window was closed
    while( !Input::actions[Input::ActionEnum::KEY_ESCAPE].clicked &&
           glfwWindowShouldClose(window) == 0 );

    // Cleanup VBO and shader
    scene.clear();
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void userInteractions(GLFWwindow *window)
{
    if(Input::actions[Input::ActionEnum::KEY_ESCAPE].clicked){
        glfwSetWindowShouldClose(window, true);
    }
    
    if(Input::actions[Input::ActionEnum::KEY_PLUS].pressed){
        scene.mountain.addVerticesQuantity(1);
    }
    if(Input::actions[Input::ActionEnum::KEY_MINUS].pressed){
        if(scene.mountain.getNumberOfVertices() > 2){
            scene.mountain.addVerticesQuantity(-1);
        }
    }

    Camera::update(deltaTime);

    // // // rotation
    // double xpos, ypos;
    // glfwGetCursorPos(window, &xpos, &ypos);
    // hAngle += mSpeed * deltaTime * float(1024/2 - xpos );
    // vAngle   += mSpeed * deltaTime * float( 768/2 - ypos );
    // camera_target = glm::vec3(
    //                                 cos(vAngle) * sin(hAngle),
    //                                 sin(vAngle),
    //                                 cos(vAngle) * cos(hAngle));
    // camera_up = glm::cross(left, camera_target);
    

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
