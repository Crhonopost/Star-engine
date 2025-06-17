#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <engine/include/API/ResourceManagement/IResource.hpp>


struct Texture: public IResource {
    std::string path;
    GLuint id;
    bool visible;
    
    Texture(): id(0), visible(false){};
    
    bool loadTextureFromData(const unsigned char* data,
                                          size_t size,
                                          int width,
                                          int height,
                                          int channels,
                                          const std::string& key);
    bool load(const std::string &name) override;
    

    static std::shared_ptr<Texture> emptyTexture;

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



struct Material: public IResource {
    glm::vec3 albedo = {1.f, 0.7f, 0.77f};
    float metallic = 0.5f;
    float roughness = 0.5f;
    float ao = 1.0f;

    std::shared_ptr<Texture> albedoTex, normalTex, metallicTex, roughnessTex, aoTex;

    Material();
    bool load(const std::string &name) override;
};


struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec4 boneWeights;
    glm::ivec4 boneIndices;
};

struct IMesh: IResource {
    virtual void draw() = 0;
    virtual ~IMesh() = default;
};

struct SingleMesh: public IMesh {
    int nbOfIndices = 0;
    GLuint VAO, VBO, EBO;

    std::shared_ptr<Material> material;

    SingleMesh() : VAO(0), VBO(0), EBO(0) {}
    SingleMesh(SingleMesh&& other) noexcept
        : nbOfIndices(other.nbOfIndices), VAO(other.VAO), VBO(other.VBO), EBO(other.EBO) {
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.nbOfIndices = 0;
    }
    SingleMesh& operator=(SingleMesh&& other) noexcept {
        if (this != &other) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);

            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;
            nbOfIndices = other.nbOfIndices;

            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
            other.nbOfIndices = 0;
        }
        return *this;
    }
    ~SingleMesh() {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
        }
        if (EBO != 0) {
            glDeleteBuffers(1, &EBO);
        }
    }

    void init(std::vector<Vertex> &vertices, std::vector<short unsigned int> &indices);
    void draw() override;
    bool load(const std::string &name) override;
};

struct MultiMesh: public IMesh {
    std::vector<std::shared_ptr<SingleMesh>> subMeshes;

    MultiMesh() = default;
    MultiMesh(MultiMesh&& other) noexcept
        : subMeshes(std::move(other.subMeshes)){
        other.subMeshes.clear();
    }
    MultiMesh& operator=(MultiMesh&& other) noexcept {
        if (this != &other) {
            subMeshes = std::move(other.subMeshes);

            other.subMeshes.clear();
        }
        return *this;
    }
    ~MultiMesh() {
        subMeshes.clear();
    }

    // Mesh(const std::string &name, const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices);

    bool load(const std::string &name) override;
    void draw() override;
};


struct MeshHelper {
    static std::shared_ptr<SingleMesh> generateCube(float sideLength, int nbOfVerticesSide, bool inward=false);
    static std::shared_ptr<SingleMesh> generatePlane(float sideLength, int nbOfVerticesSide);
    static std::shared_ptr<SingleMesh> generateSphere(float radius, int nbOfVerticesTotal = 1000);
};


class Program {
    private:
    GLuint modelLocation, vLocation, pLocation;

    protected:
    // Setup à l'initialisation uniquement
    std::unordered_map<GLuint, std::shared_ptr<Texture>> programTextures;
    
    public:
    GLuint programID;

    Program() = default;
    Program(const char *vertexPath, const char *fragmentPath);

    virtual void beforeRender();
    virtual void afterRender();
    
    void clear();
    void renderTextures();
    void initTexture(char *path, char *uniformName);
    virtual void updateGUI();

    void updateViewMatrix(const glm::mat4 &v);
    void updateProjectionMatrix(const glm::mat4 &p);
    void updateModelMatrix(const glm::mat4 &model);

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


