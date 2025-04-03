#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <functional>


struct Texture {
    char *path, *uniformName;
    GLuint id, uniformLocation;

    Texture(): id(0){};

    ~Texture() {
        if (id != 0) {
            glDeleteTextures(1, &id);
        }
    }
};

struct Drawable {
    GLuint VAO, VBO, EBO;
    int indexCount;

    std::vector<Texture> textures;
    int programIdx;

    
    bool isLod = false;
    Drawable* lodLower = nullptr;
    float switchDistance = -1.0f;

    Drawable(): VAO(0), VBO(0), EBO(0){};

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

    void init(std::vector<float>&, std::vector<short unsigned int>&);
    void draw(float renderDistance);
};

enum RotationOrderEnum {
    XYZ,
    YXZ,
    ZYX
};
class Transform {
    private:
    glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
    glm::vec3 eulerRot = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    glm::mat4 modelMatrix = glm::mat4(1.f);
    bool dirty = true;

    glm::mat4 getLocalModelMatrix();

    public:
    RotationOrderEnum rotationOrder = YXZ;
    
    void computeModelMatrix();
    
    void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix);

    bool isDirty();
    
    void setLocalPosition(glm::vec3 position);

    glm::vec3 getLocalPosition();
    
    void setLocalRotation(glm::vec3 rotationAngles);

    void rotate(glm::vec3 rotations);
    void translate(glm::vec3);

    glm::mat4 getModelMatrix();
};

struct CustomBehavior {
    std::function<void(float)> update;
};
