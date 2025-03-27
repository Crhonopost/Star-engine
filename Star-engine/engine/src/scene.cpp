#include <engine/include/scene.hpp>
#include <engine/include/program.hpp>

SpatialNode *sun, *earthOrbit, *earth, *moonOrbit, *moon;

void SceneGraph::init() {
    Program planetProg("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");
    TextureData sunTexture = planetProg.loadTexture("../assets/images/planets/tex_sun.jpg", "tex");
    TextureData earthTexture = planetProg.loadTexture("../assets/images/planets/tex_earth.jpg", "tex");
    TextureData moonTexture = planetProg.loadTexture("../assets/images/planets/tex_moon.jpg", "tex");
    
    auto sunSphere = new Sphere(1.f);
    sunSphere->texture = sunTexture;
    
    auto earthSphere = new Sphere(0.7f);
    earthSphere->texture = earthTexture;
    auto spatialEarth = new SpatialNode();
    
    auto moonSphere = new Sphere(0.25f);
    moonSphere->texture = moonTexture;
    auto spatialMoon = new SpatialNode();
    
    
    spatialEarth->transform.setLocalPosition(glm::vec3(4.f, 0.f, 0.f));
    earthSphere->transform.setLocalRotation(glm::vec3(0.f, 0.f, -23.44f));
    earthSphere->transform.rotationOrder = ZYX;
    moonSphere->transform.setLocalPosition(glm::vec3(2.f, 0.f, 0.f));
    moonSphere->transform.setLocalRotation(glm::vec3(0,0,6.68));
    moonSphere->transform.rotationOrder = ZYX;
    spatialMoon->transform.setLocalRotation(glm::vec3(0,0,5.14));
    
    
    
    sun = sunSphere;
    earthOrbit = spatialEarth;
    earth = earthSphere;
    moonOrbit = spatialMoon;
    moon = moonSphere;
    
    AddChild(sunSphere);
    AddChild(spatialEarth);
    spatialEarth->AddChild(earthSphere);
    spatialEarth->AddChild(spatialMoon);
    spatialMoon->AddChild(moonSphere);
    
    planetProg.meshes.push_back(sunSphere);
    planetProg.meshes.push_back(earthSphere);
    planetProg.meshes.push_back(moonSphere);
    
    programs.push_back(planetProg);
    
    updateSelfAndChildTransform();
}


void SceneGraph::update(double delta){
    transform.rotate(glm::vec3(0.f, delta * 10.5f, 0.f));
    earthOrbit->transform.rotate(glm::vec3(0.f, -delta * 10.5f, 0.f));
    earth->transform.rotate(glm::vec3(0.f, delta * 20.5f , 0.f ));
    moonOrbit->transform.rotate(glm::vec3(0, -delta * 10.5f, 0));
    moon->transform.rotate(glm::vec3(0, delta * 5.f, 0.f));
    
    // glUniform1f(timeLocation, (float) delta);
    updateSelfAndChildTransform();
}


void SceneGraph::render(){
    glm::mat4 vp = Camera::getInstance().getVP();
    for(auto &prog : programs){
        glUseProgram(prog.programID);
        prog.updateViewProjectionMatrix(vp);
        prog.render();
    }
}

void SceneGraph::clear(){
    for(auto &prog : programs){
        prog.clear();
    }

    for(auto &child : children_){
        free(child);
    }
}