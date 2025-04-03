// #include <engine/include/scene.hpp>
// #include <engine/include/program.hpp>


// SpatialNode *player;

// void SceneGraph::init() {
//     Program baseProg("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
//     Program mountainProg("shaders/vertex_shader_mountain.glsl", "shaders/fragment_shader_mountain.glsl");
    
//     TextureData sunTexture = baseProg.loadTexture("../assets/images/planets/tex_sun.jpg", "tex");

//     TextureData heightTexture = mountainProg.loadTexture("../assets/images/HeightMap.png", "heightMap");
//     TextureData grassTexture = mountainProg.loadTexture("../assets/images/grass.png", "texGrass");
//     TextureData rockTexture = mountainProg.loadTexture("../assets/images/rock.png", "texRock");
//     TextureData snowTexture = mountainProg.loadTexture("../assets/images/snowrocks.png", "texSnow");

//     auto sunSphere = new Sphere(0.25f);
//     sunSphere->transform.setLocalPosition(glm::vec3(0,1,0));
//     sunSphere->textures.push_back(sunTexture);
//     player = sunSphere;

//     auto mountain = new Mountain(50, 50, 128);
//     mountain->textures.push_back(heightTexture);
//     mountain->textures.push_back(grassTexture);
//     mountain->textures.push_back(rockTexture);
//     mountain->textures.push_back(snowTexture);

    
//     AddChild(sunSphere);
//     AddChild(mountain);

//     baseProg.meshes.push_back(sunSphere);
//     mountainProg.meshes.push_back(mountain);
    
//     programs.push_back(baseProg);
//     programs.push_back(mountainProg);



//     Camera::getInstance().camera_position = glm::vec3(0, 7, 20);
    
//     updateSelfAndChildTransform();
// }


// void SceneGraph::update(double delta){
//     // glUniform1f(timeLocation, (float) delta);

//     glm::vec3 target = player->transform.getLocalPosition() - Camera::getInstance().camera_position;
//     Camera::getInstance().camera_target = glm::normalize(target);

//     updateSelfAndChildTransform();
// }


// void SceneGraph::render(){
//     glm::mat4 vp = Camera::getInstance().getVP();
//     for(auto &prog : programs){
//         glUseProgram(prog.programID);
//         prog.updateViewProjectionMatrix(vp);
//         prog.render();
//     }
// }

// void SceneGraph::clear(){
//     for(auto &prog : programs){
//         prog.clear();
//     }

//     for(auto &child : children_){
//         free(child);
//     }
// }