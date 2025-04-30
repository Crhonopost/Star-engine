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

#include <backend/imgui_impl_glfw.h>

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

// personnals
#include <engine/include/input.hpp>
#include <engine/include/rendering.hpp>
#include <engine/include/ecs/ecsManager.hpp>
#include <engine/include/ecs/implementations/components.hpp>
#include <engine/include/ecs/implementations/systems.hpp>
#include <engine/include/camera.hpp>
#include <engine/include/spatial.hpp>
#include <engine/include/stbi.h>
#include <engine/include/scene.hpp>
#include <imgui.h>
#include <backend/imgui_impl_opengl3.h>
#include <common/json.hpp>
#include <engine/include/API/FileSystem.hpp>


void userInteractions(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);


using json = nlohmann::json;
using namespace nlohmann::literals;


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

unsigned int currentWidth = SCR_WIDTH;
unsigned int currentHeight = SCR_HEIGHT;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

//rotation
float angle = 0.;
float zoom = 1.;
/*******************************************************************************/

bool isInEditor = true;

// SceneGraph scene;
SpatialNode root;

// ecs
ecsManager ecs;
std::shared_ptr<Render> renderSystem;
std::shared_ptr<PBRrender> pbrRenderSystem;
std::shared_ptr<LightRender> lightRenderSystem;
std::shared_ptr<CustomSystem> customSystem;
std::shared_ptr<CollisionDetectionSystem> collisionDetectionSystem;
std::shared_ptr<PhysicSystem> physicSystem;
std::shared_ptr<PhysicDebugSystem> physicDebugSystem;


void initEcs(){
    ecs.Init();
    
    ecs.RegisterComponent<Transform>("Transform");
    ecs.RegisterComponent<Drawable>("Drawable");
    ecs.RegisterComponent<CustomProgram>("CustomProgram");
    ecs.RegisterComponent<Material>("Material");
    ecs.RegisterComponent<Light>("Light");
    ecs.RegisterComponent<CustomBehavior>("CustomBehavior");
    ecs.RegisterComponent<RigidBody>("RigidBody");
    ecs.RegisterComponent<CollisionShape>("CollisionShape");

    renderSystem = ecs.RegisterSystem<Render>();
    pbrRenderSystem = ecs.RegisterSystem<PBRrender>();
    lightRenderSystem = ecs.RegisterSystem<LightRender>();
    customSystem = ecs.RegisterSystem<CustomSystem>();
    collisionDetectionSystem = ecs.RegisterSystem<CollisionDetectionSystem>();
    physicSystem = ecs.RegisterSystem<PhysicSystem>();
    physicDebugSystem = ecs.RegisterSystem<PhysicDebugSystem>();
    physicDebugSystem->init();
    
    Signature renderSignature;
    renderSignature.set(ecs.GetComponentType<Transform>());
    renderSignature.set(ecs.GetComponentType<Drawable>());
    renderSignature.set(ecs.GetComponentType<CustomProgram>());
    ecs.SetSystemSignature<Render>(renderSignature);

    Signature pbrRenderSignature;
    pbrRenderSignature.set(ecs.GetComponentType<Transform>());
    pbrRenderSignature.set(ecs.GetComponentType<Drawable>());
    pbrRenderSignature.set(ecs.GetComponentType<Material>());
    ecs.SetSystemSignature<PBRrender>(pbrRenderSignature);

    Signature lightSignature;
    lightSignature.set(ecs.GetComponentType<Transform>());
    lightSignature.set(ecs.GetComponentType<Light>());
    ecs.SetSystemSignature<LightRender>(lightSignature);

    Signature customSignature;
    customSignature.set(ecs.GetComponentType<CustomBehavior>());
    ecs.SetSystemSignature<CustomSystem>(customSignature);

    Signature collisionDetectionSignature;
    collisionDetectionSignature.set(ecs.GetComponentType<Transform>());
    collisionDetectionSignature.set(ecs.GetComponentType<CollisionShape>());
    ecs.SetSystemSignature<CollisionDetectionSystem>(collisionDetectionSignature);

    Signature physicSignature;
    physicSignature.set(ecs.GetComponentType<RigidBody>());
    physicSignature.set(ecs.GetComponentType<Transform>());        
    physicSignature.set(ecs.GetComponentType<CollisionShape>());        
    ecs.SetSystemSignature<PhysicSystem>(physicSignature);

    Signature physicDebugSignature;
    physicDebugSignature.set(ecs.GetComponentType<Transform>());        
    physicDebugSignature.set(ecs.GetComponentType<CollisionShape>());        
    ecs.SetSystemSignature<PhysicDebugSystem>(physicDebugSignature);
}


void unloadScene(){
    root.destroy();
    ecs.DestroyAllEntities();
    Program::destroyPrograms();
}

void afterSceneInit(){
    pbrRenderSystem->initPBR();

    root.updateSelfAndChildTransform();

    Program *pbr = Program::programs.back().get();

    Program::programs.push_back(std::make_unique<Skybox>());
    Entity skyboxEntity = ecs.CreateEntity();
    ecs.SetEntityName(skyboxEntity, "Skybox");
    Drawable skyboxDraw = Render::generateCube(9999, 2, true);
    Transform skyboxTransform;
    CustomProgram skyboxProg(Program::programs[Program::programs.size()-1].get());

    ecs.AddComponent<Transform>(skyboxEntity, skyboxTransform);
    ecs.AddComponent<Drawable>(skyboxEntity, skyboxDraw);
    ecs.AddComponent<CustomProgram>(skyboxEntity, skyboxProg);


    lightRenderSystem->update();


    CubemapRender irradianceMapRender(512);
    // Render scene into a cubemap
    irradianceMapRender.renderFromPoint({0,5,0}, renderSystem.get(), pbrRenderSystem.get());

    GLuint octa;
    irradianceMapRender.unwrapOctaProj(octa, 512, (Skybox*) skyboxProg.programPtr);
    
    auto irradianceShader = std::make_unique<IrradianceShader>();        
    Cubemap irradianceMap(32);

    // Apply shader onto skybox
    irradianceMapRender.applyFilter(irradianceShader.get(), irradianceMap);
    pbrRenderSystem->setIrradianceMap(irradianceMap.textureID);



    
    auto testCubemapRenderEntity = ecs.CreateEntity();
    ecs.SetEntityName(testCubemapRenderEntity, "Cubemap visu");
    Drawable cubemapDraw = Render::generateCube(2, 2, false);
    Transform cubemapTransform;
    cubemapTransform.translate({0,5,0});

    Program::programs.push_back(std::make_unique<CubemapProg>());
    CustomProgram cubemapProg(Program::programs[Program::programs.size()-1].get());
    ((CubemapProg*) cubemapProg.programPtr)->textureID = irradianceMap.textureID;
    ecs.AddComponent<Transform>(testCubemapRenderEntity, cubemapTransform);
    ecs.AddComponent<Drawable>(testCubemapRenderEntity, cubemapDraw);
    ecs.AddComponent<CustomProgram>(testCubemapRenderEntity, cubemapProg);

    std::unique_ptr<SpatialNode> cubemapNode = std::make_unique<SpatialNode>(&ecs.GetComponent<Transform>(testCubemapRenderEntity));
    root.AddChild(std::move(cubemapNode));
}

void switchEditorMode(){
    glm::vec3 target = Camera::getInstance().camera_target;
    glm::vec3 position = Camera::getInstance().camera_position;
    
    isInEditor = !isInEditor;
    Camera::editor = isInEditor;
    
    Camera::getInstance().camera_target = target;
    Camera::getInstance().camera_position = position;
}

void editorUpdate(float deltaTime){
    if(ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoCollapse)) {
        if(ImGui::Begin("Shaders")){
            for(auto &prog: Program::programs){
                prog->updateGUI();
            }
        }
        ImGui::End();

        if (ImGui::Button("Switch mode")) switchEditorMode();

        if (ImGui::Button("Load scene 1")){
            unloadScene();
            initScene(root, ecs);
            afterSceneInit();
        } else if(ImGui::Button("Load scene 2")){
            unloadScene();
            pbrScene(root, ecs);
            afterSceneInit();
        }

        ecs.DisplayUI();
    }

    bool savePicture = ImGui::Button("save ppm");

    if(ImGui::Button("json test")){
        json state;
        for(uint32_t entity = 0; entity<ecs.getEntityCount(); entity ++){
            json entityValue = {{"name", ecs.GetEntityName(entity)}};
            for (auto& inspector : ecs.componentInspectors) {
                auto res = inspector->GetJson(ecs, entity);
                if(!res.empty())
                    entityValue["components"].push_back(res);
            }
            state["entities"].push_back(entityValue);
        }
        writeToFile("scenes/test.json", state);
    }
    ImGui::End();

    
    Camera::getInstance().updateInput(deltaTime);
    glm::mat4 view = Camera::getInstance().getV();
    lightRenderSystem->update();
    renderSystem->update(view);
    pbrRenderSystem->update(view);
    
    physicDebugSystem->update();

    if(savePicture)  save_PPM_file(SCR_WIDTH, SCR_HEIGHT, "../pictures/scene.ppm");
}

void gameUpdate(float deltaTime){
    glm::mat4 view = Camera::getInstance().getV();

    customSystem->update(deltaTime);
    collisionDetectionSystem->update(deltaTime);
    physicSystem->update(deltaTime);
    lightRenderSystem->update();
    renderSystem->update(view);
    pbrRenderSystem->update(view);
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
        glfwSetWindowSizeCallback(window, framebuffer_size_callback);
        //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
        glfwPollEvents();
        glfwSetCursorPos(window, 1024/2, 768/2);
    
        glClearColor(0.f, 1.f, 0.2f, 0.0f);
    
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    
        //glEnable(GL_CULL_FACE);
    
        // For speed computation
        int nbFrames = 0;
    
        
        initEcs();
        auto actions = InputManager::getInstance().getActions();

        // initScene(root, ecs);
        pbrScene(root, ecs);
    
        afterSceneInit();

        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
        
        
        do{
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
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

            if(actions[InputManager::EDITOR_SWITCH_MODE].clicked) switchEditorMode();

    
            if(isInEditor) editorUpdate(deltaTime);
            else gameUpdate(deltaTime);
            Texture::resetActivationInt();
            
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Swap buffers
            glfwSwapBuffers(window);
            glfwPollEvents();

    
        } // Check if the ESC key was pressed or the window was closed
        while( !actions[InputManager::ActionEnum::KEY_ESCAPE].clicked &&
               glfwWindowShouldClose(window) == 0 );
    
        // scene.clear();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    
        for(auto &prog: Program::programs){
            prog->clear();
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
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    currentWidth = width;
    currentHeight = height;
    glViewport(0, 0, width, height);
}
