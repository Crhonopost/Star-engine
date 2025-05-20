#pragma once

#include <vector>
#include <map>
#include <memory>

#include <engine/include/ecs/implementations/components.hpp>
#include <engine/include/ecs/implementations/componentInspector.hpp>
#include <engine/include/animation.hpp>

class Drawable;

class Material;

struct Texture {
    const char *path;
    GLuint id;
    bool visible;
    
    Texture(): id(0), visible(false){};
    
    static Texture& loadTextureFromMemory(const unsigned char* data,
                                          size_t size,
                                          int width,
                                          int height,
                                          int channels,
                                          const std::string& key);
    static Texture& loadTexture(const char * path);
    static std::map<std::string, Texture> textures;

    static Texture emptyTexture;
    
    void activate(GLuint textureLocation);
    static int getAvailableActivationInt();
    static void resetActivationInt();

    private:
    static int activationInt;
};


struct Cubemap {
    GLuint textureID;
    int resolution;
    Cubemap(int resolution);
    Cubemap(std::vector<std::string> paths);
    void clear();
};

class Program {
    private:
    GLuint modelLocation, vLocation, pLocation;

    protected:
    // Setup à l'initialisation uniquement
    std::map<GLuint, Texture*> programTextures;
    
    public:
    GLuint programID;
    static std::vector<std::unique_ptr<Program>> programs;

    Program() = default;
    Program(const char *vertexPath, const char *fragmentPath);

    virtual void beforeRender();
    virtual void afterRender();
    
    void clear();
    void renderTextures();
    void initTexture(char *path, char *uniformName);
    virtual void updateGUI();

    void updateViewMatrix(glm::mat4 &v);
    void updateProjectionMatrix(glm::mat4 &p);
    void updateModelMatrix(glm::mat4 model);

    void updateLightCount(int count);
    void updateLightPosition(int lightIndex, glm::vec3 position);
    void updateLightColor(int lightIndex, glm::vec3 color);

    void use() {
        glUseProgram(programID);
    }
    void setFloat(char* uniformName,float valeur){
        glUniform1f(glGetUniformLocation(programID,uniformName),valeur);
    }
    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value); 
    }
    static void destroyPrograms();
};

class Skybox: public Program{
    public:
    GLuint octaProjLoc;
    Cubemap cubemap;
    Skybox(Cubemap sky);
    
    void beforeRender() override;
    void afterRender() override;
    void setProjectionOcta(bool state);
};

class IrradianceShader:public Program{
    public:
    IrradianceShader();
    
};
class PrefilterShader:public Program{
    public:
    PrefilterShader();
};

class BrdfShader:public Program{
    public:
    BrdfShader();
};

class CubemapProg: public Program {
    public:
    GLuint textureID;

    CubemapProg();

    void beforeRender() override;
};

class PBR: public Program{
    private:
    GLuint albedoLocation, metallicLocation, roughnessLocation, aoLocation, hasTextureLocation, indensiteScaleLightLocation;
    GLuint albedoTexLocation, metallicTexLocation, roughnessTexLocation, aoTexLocation, normalTexLocation;
    GLuint hasAlbedoMapLocation, hasNormalMapLocation, hasMetallicMapLocation, hasRoughnessMapLocation, hasAoMapLocation;
    

    public:
    PBR();
    void updateGUI() override;
    void updateMaterial(Material &value);

    void updateLightCount(int count);
    void updateLightPosition(int lightIndex, glm::vec3 position);
    void updateLightColor(int lightIndex, glm::vec3 color);
    
};

class ProbeProg: public Program{
    public:
    GLuint probeCountsLoc, probeStartPositionLoc, probeStepLoc, lowResolutionDownsampleFactorLoc;
    ProbeProg();
};

void save_PPM_file(int width, int height, const std::string& filename);




// COMPONENTS
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec4 boneWeights;
    glm::ivec4 boneIndices;
};

struct Drawable: Component {
    GLuint VAO, VBO, EBO;
    int indexCount;

    Drawable* lodLower = nullptr;
    float switchDistance = -1.0f;

    Drawable(): VAO(0), VBO(0), EBO(0){};

    Drawable(Drawable&& other) noexcept
        : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), 
          indexCount(other.indexCount),
          lodLower(other.lodLower), switchDistance(other.switchDistance) {
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }

    Drawable& operator=(Drawable&& other) noexcept {
        if (this != &other) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);

            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;
            indexCount = other.indexCount;
            lodLower = other.lodLower;
            switchDistance = other.switchDistance;

            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
        }
        return *this;
    }

    ~Drawable() {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
        }
        if (EBO != 0) {
            glDeleteBuffers(1, &EBO);
        }
        // Les textures seront automatiquement détruites grâce au destructeur de std::vector
    }

    void init(std::vector<Vertex>&, std::vector<short unsigned int>&);
    void draw(float renderDistance);
};

struct AnimatedDrawable: Drawable{
    bool playing = false;
    std::vector<Bone> bones;
    Animation animation;
};

struct Material: Component {
    glm::vec3 albedo = {1.f, 0.7f, 0.77f};
    float metallic = 0.5f;
    float roughness = 0.5f;
    float ao = 1.0f;

    Texture *albedoTex, *normalTex, *metallicTex, *roughnessTex, *aoTex;

    Material();
};

struct Light: Component {
    glm::vec3 color;
    float strength;
    // Cubemap depthCubemap;
    GLuint depthID; 
    GLuint shaderLoc;

    Light();
};

struct CustomProgram: Component {
    Program *programPtr;
    CustomProgram():Component(){};
    CustomProgram(Program *progPtr);
};



// INSPECTORS
template<>
inline void ComponentInspector<Drawable>::DisplayComponentGUI(Drawable& drawable){
    ImGui::SeparatorText("Drawable");
    ImGui::DragFloat("Lod switch distance", &drawable.switchDistance);
}

template<>
inline void ComponentInspector<AnimatedDrawable>::DisplayComponentGUI(AnimatedDrawable& drawable){
    ImGui::SeparatorText("Animated Drawable");
    ImGui::DragFloat("Lod switch distance", &drawable.switchDistance);
    if(ImGui::Checkbox("Playing", &drawable.playing));
    ImGui::DragFloat("Animation time", &drawable.animation.currentTime, 0.01f, 0.0f, drawable.animation.duration / 1000.f);
}

template<>
inline void ComponentInspector<CustomProgram>::DisplayComponentGUI(CustomProgram& prog){}


template<>
inline void ComponentInspector<Material>::DisplayComponentGUI(Material& material){
    ImGui::SeparatorText("Material");
    
    ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 5.0f);
    ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 5.0f);
    ImGui::SliderFloat("Ambient oclusion", &material.ao, 0.0f, 5.0f);
    ImGui::SliderFloat3("Albedo", &material.albedo[0], 0.f, 1.f);
    
    // ImGui::Checkbox("use textures",&material.hasTexture);


    // if(ImGui::SliderFloat("indensity of light", &indensiteScaleLight, 1.0f, 500.0f)){
    //     glUniform1f(indensiteScaleLightLocation, indensiteScaleLight);
    // }

    // const char* items[] = { "gold", "oldMetal", "rock", "woods" };
    // static int selected = currentMaterialIndex;
    // if (ImGui::Combo("Material Folder", &selected, items, IM_ARRAYSIZE(items))) {
    //     currentMaterialIndex = selected;
    //     loadCurrentMaterial();
    // }
}

template<>
inline void ComponentInspector<Light>::DisplayComponentGUI(Light& light){
    ImGui::SeparatorText("Light");
    ImGui::DragFloat3("Color", &light.color.x, 0.01f, 0.0f, 1.0f);
}



// Json
template<>
inline json ComponentInspector<Drawable>::GetComponentJson(Drawable& drawable){
    // TODO: store mesh path for drawable and reference lod lower
    return {
        {"name", "Drawable"}, 
        {"data", {
            {"switch_distance", drawable.switchDistance}}
        }
    };
}

template<>
inline json ComponentInspector<AnimatedDrawable>::GetComponentJson(AnimatedDrawable& drawable){
    // TODO: store mesh path for drawable and reference lod lower
    return {
        {"name", "AnimatedDrawable"}
    };
}

template<>
inline json ComponentInspector<CustomProgram>::GetComponentJson(CustomProgram& prog){
    return {{"name", "CustomProg"}};
}


template<>
inline json ComponentInspector<Material>::GetComponentJson(Material& material){
    return {
        {"name", "Material"},
        {"data", {
            {"ao", material.ao},
            {"metallic", material.metallic},
            {"roughness", material.roughness},
            // {"albedo", serializeVec3(material.albedo)}
        }}
    };
}

template<>
inline json ComponentInspector<Light>::GetComponentJson(Light& drawable){
    return {{"name", "Light"}};
}
