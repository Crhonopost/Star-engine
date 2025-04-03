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

int textureWidth, textureHeight, textureChannels;
unsigned char *textureData = stbi_load("../assets/images/HeightMap.png", &textureWidth, &textureHeight, &textureChannels, 0);
float getHeightValue(glm::vec3 position, glm::vec3 meshPosition, glm::vec2 meshDimmensions) {
    if (!textureData) {
        std::cerr << "Failed to load heightmap texture!" << std::endl;
        return 0.0f;
    }
    
    glm::vec2 localPos = glm::vec2(position.x, position.z) - glm::vec2(meshPosition.x - meshDimmensions.x/2.0f, meshPosition.z - meshDimmensions.y/2.0f);
    
    glm::vec2 uv = localPos / meshDimmensions;
    
    // Vérifier si la position est dans les bornes de la heightmap
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
        std::cout << "hors limite\n";
        return 0.0f; // En dehors des limites
    }
    
    // Convertir les UV en coordonnées de texture
    int texX = textureWidth - static_cast<int>(uv.x * (textureWidth - 1));
    int texY = static_cast<int>(uv.y * (textureHeight - 1));
    
    // Accéder à la hauteur dans l'image (supposons une texture en niveaux de gris)
    int pixelIndex = (texY * textureWidth + texX) * textureChannels;
    unsigned char heightValue = textureData[pixelIndex];
    
    // Normaliser la hauteur entre 0 et 1
    return heightValue / 255.0f;
}


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
    
        auto renderSystem = ecs.RegisterSystem<Render>();
        auto customSystem = ecs.RegisterSystem<CustomSystem>();
        
        Signature renderSignature;
        renderSignature.set(ecs.GetComponentType<Transform>());
        renderSignature.set(ecs.GetComponentType<Drawable>());
        ecs.SetSystemSignature<Render>(renderSignature);
    
        Signature customSignature;
        customSignature.set(ecs.GetComponentType<CustomBehavior>());
        ecs.SetSystemSignature<CustomSystem>(customSignature);
    
    
        ///////////////////////////// programs
        Program baseProg("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
        Texture sunTexture = baseProg.loadTexture("../assets/images/planets/tex_sun.jpg", "tex");
    
    
        Program mountainProg("shaders/vertex_shader_mountain.glsl", "shaders/fragment_shader.glsl");
        Texture grassTex = mountainProg.loadTexture("../assets/images/HeightMap.png", "tex");
        // Texture rockTex = mountainProg.loadTexture("../assets/images/rock.png", "texRock");
        Texture heightMap = mountainProg.loadTexture("../assets/images/HeightMap.png", "heightMap");
    
        Program::programs.push_back(baseProg);
        Program::programs.push_back(mountainProg);




        ////////////////////////////// mountain
        auto mountainEntity = ecs.CreateEntity();
        const int sideLength = 100;
        auto mountainDraw = Render::generatePlane(sideLength,256);
        mountainDraw.programIdx = 1;
        mountainDraw.textures.push_back(heightMap);
        mountainDraw.textures.push_back(grassTex);
        // mountainDraw.textures.push_back(rockTex);
        Transform mountainTransform;
        ecs.AddComponent(mountainEntity, mountainDraw);
        ecs.AddComponent(mountainEntity, mountainTransform);



        
    
        ///////////////////////////// sun
        auto sunEntity = ecs.CreateEntity();
        auto sunDraw = Render::generateSphere(0.5f);
        sunDraw.programIdx = 0;
        sunDraw.textures.push_back(sunTexture);
        Transform sunTransform;
        
        CustomBehavior sunBehavior;
        sunBehavior.update = [&sunEntity, &actions, &mountainEntity, &sideLength](float deltaTime) {

            const glm::vec3 left = glm::cross(glm::vec3(0,1,0), Camera::getInstance().camera_target);
            const glm::vec3 forward = glm::cross(left, glm::vec3(0,1,0));
            
            const float speed = 10.0f;
            auto &sunTransform = ecs.GetComponent<Transform>(sunEntity);
            if (actions[InputManager::ActionEnum::ACTION_FORWARD].pressed)
                sunTransform.translate(forward * speed * deltaTime);
            if (actions[InputManager::ActionEnum::ACTION_BACKWARD].pressed)
                sunTransform.translate(-forward * speed * deltaTime);
            if (actions[InputManager::ActionEnum::ACTION_LEFT].pressed)
                sunTransform.translate(left * speed * deltaTime);
            if (actions[InputManager::ActionEnum::ACTION_RIGHT].pressed)
                sunTransform.translate(-left * speed * deltaTime);

            glm::vec3 mountainPos = ecs.GetComponent<Transform>(mountainEntity).getLocalPosition();
            float heightValue = getHeightValue(sunTransform.getLocalPosition(), mountainPos, glm::vec2(sideLength, sideLength));
            const float scale = 20.0f;
            sunTransform.translate(glm::vec3(0,heightValue * scale - sunTransform.getLocalPosition().y + 0.5,0));
        };

        Drawable lowerRes = Render::generatePlane(1, 2);
        sunDraw.lodLower = &lowerRes;
        sunDraw.switchDistance = 15;

        ecs.AddComponent(sunEntity, sunDraw);
        ecs.AddComponent(sunEntity, sunTransform);
        ecs.AddComponent(sunEntity, sunBehavior);


        

        

        
        ///////////////////////////// camera
        auto cameraEntity = ecs.CreateEntity();
        CustomBehavior cameraUpdate;
        cameraUpdate.update = [&sunEntity,  &actions](float deltaTime){
            glm::vec3 sunPos = ecs.GetComponent<Transform>(sunEntity).getLocalPosition();
            glm::vec3 newTarget = glm::normalize(sunPos - Camera::getInstance().camera_position);

            Camera::getInstance().camera_target = newTarget;

            if (actions[InputManager::ActionEnum::ACTION_LOCK_POSITION].clicked)
                Camera::getInstance().locked = !Camera::getInstance().locked;

            if(!Camera::getInstance().locked){
                glm::vec3 newPos = sunPos - newTarget * 10.f;
                newPos.y = sunPos.y + 5.0f;
                Camera::getInstance().camera_position = glm::mix(Camera::getInstance().camera_position, newPos, deltaTime);
            }
            // Camera::getInstance().updateInput(deltaTime);
        };
        ecs.AddComponent(cameraEntity, cameraUpdate);


        auto root = spatial::SpatialNode();
        Transform rootTransform;
        root.transform = &rootTransform;
        
        auto sunNode = spatial::SpatialNode();
        sunNode.transform = &ecs.GetComponent<Transform>(sunEntity);
        auto mountainNode = spatial::SpatialNode();        
        mountainNode.transform = &ecs.GetComponent<Transform>(mountainEntity);
        
        root.AddChild(&sunNode);
        root.AddChild(&mountainNode);
        
        
        do{
            // sunTransform.computeModelMatrix();
            // error = glGetError();
            // if(error != GL_NO_ERROR){
            //     std::cerr << "OpenGL error: " << error << std::endl;
            // } 
            
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            
            // input
            InputManager::getInstance().processInput(window);
            userInteractions(window);
            actions = InputManager::getInstance().getActions();
            
    
            // scene.update(deltaTime);
            
            // Clear the screen
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            
            // scene.render();

            root.updateSelfAndChildTransform();
    
            customSystem->update(deltaTime);
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
