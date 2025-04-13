#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <functional>
#include <iostream>
#include <engine/include/ecs/base/entity.hpp>


struct Texture {
    char *path, *uniformName;
    GLuint id, uniformLocation;

    Texture(): id(0){};
};

struct Component{
    Component(const Component&) = delete;
    Component() {}
};

struct Drawable: Component {
    GLuint VAO, VBO, EBO;
    int indexCount;

    std::vector<Texture> textures;
    int programIdx;

    Drawable* lodLower = nullptr;
    float switchDistance = -1.0f;

    Drawable(): VAO(0), VBO(0), EBO(0){};

    Drawable(Drawable&& other) noexcept
        : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), 
          indexCount(other.indexCount), textures(std::move(other.textures)),
          programIdx(other.programIdx), lodLower(other.lodLower), switchDistance(other.switchDistance) {
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
            textures = std::move(other.textures);
            programIdx = other.programIdx;
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

    void init(std::vector<float>&, std::vector<short unsigned int>&);
    void draw(float renderDistance);
};

enum RotationOrderEnum {
    XYZ,
    YXZ,
    ZYX
};
class Transform: Component {
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


    Transform()
    : modelMatrix(1.f), pos(0.0f), scale(1.0f), eulerRot(0.0f), rotationOrder(XYZ), dirty(true){}

    Transform(Transform&& other) noexcept
        : pos(std::move(other.pos)), 
          eulerRot(std::move(other.eulerRot)),
          scale(std::move(other.scale)),
          modelMatrix(std::move(other.modelMatrix)),
          dirty(other.dirty),
          rotationOrder(other.rotationOrder)
    {
        other.dirty = true;
    }

    Transform& operator=(Transform&& other) noexcept {
        if (this != &other) {
            pos = std::move(other.pos);
            eulerRot = std::move(other.eulerRot);
            scale = std::move(other.scale);
            modelMatrix = std::move(other.modelMatrix);
            dirty = other.dirty;
            rotationOrder = other.rotationOrder;

            other.dirty = true;
        }
        return *this;
    }
};


struct CustomBehavior: Component {
    std::function<void(float)> update;

    CustomBehavior() = default;

    CustomBehavior(CustomBehavior&& other) noexcept
        : update(std::move(other.update)) {
    }

    CustomBehavior& operator=(CustomBehavior&& other) noexcept {
        if (this != &other) {
            update = std::move(other.update);
        }
        return *this;
    }
};



////////////  Physic

struct RigidBody: Component {
    bool isStatic = false;
    glm::vec3 velocity=glm::vec3(0);
    float weight=1.f;
    float restitutionCoef=0.5f;
    float frictionCoef=0.5f;

    RigidBody() = default;

    RigidBody(RigidBody&& other) noexcept
        : isStatic(other.isStatic),
          velocity(std::move(other.velocity)),
          weight(other.weight),
          restitutionCoef(other.restitutionCoef),
          frictionCoef(other.frictionCoef)
    {
        other.weight = 1.f; // Ou autre valeur par défaut
        other.restitutionCoef = 0.5f;
        other.frictionCoef = 0.5f;
        other.isStatic = false;
    }

    RigidBody& operator=(RigidBody&& other) noexcept {
        if (this != &other) {
            isStatic = other.isStatic;
            velocity = std::move(other.velocity);
            weight = other.weight;
            restitutionCoef = other.restitutionCoef;
            frictionCoef = other.frictionCoef;

            other.weight = 1.f;
            other.restitutionCoef = 0.5f;
            other.frictionCoef = 0.5f;
            other.isStatic = false;
        }
        return *this;
    }
};



struct OverlapingShape {
    bool exist = false;
    Entity entityA, entityB;
    bool aSeeB, bSeeA;
    glm::vec3 position, normal;
    float correctionDepth;
};

enum CollisionShapeTypeEnum {
    RAY,
    SPHERE,
    PLANE
};

struct Ray {
    Ray() = default;
    glm::vec3 ray_direction;
    float length;
};

struct Sphere {
    Sphere() = default;
    float radius;
};

struct Plane {
    Plane() = default;
    glm::vec3 normal;
};


struct CollisionShape: Component{
    CollisionShapeTypeEnum shapeType;
    union {
        Ray ray;
        Sphere sphere;
        Plane plane;
    };
    uint16_t layer = 1;
    uint16_t mask = 1;

    bool isColliding=false;

    CollisionShape() : shapeType(PLANE), isColliding(false) {
        new(&plane) Plane();
    }

    CollisionShape(CollisionShape&& other) noexcept 
        : shapeType(other.shapeType), isColliding(other.isColliding) {
        switch (other.shapeType) {
            case RAY:
                new(&ray) Ray(std::move(other.ray));
                break;
            case SPHERE:
                new(&sphere) Sphere(std::move(other.sphere));
                break;
            case PLANE:
                new(&plane) Plane(std::move(other.plane));
                break;
        }
        other.isColliding = false;
    }

    CollisionShape& operator=(CollisionShape&& other) noexcept {
        if (this != &other) {
            shapeType = other.shapeType;
            isColliding = other.isColliding;

            switch (other.shapeType) {
                case RAY:
                    new(&ray) Ray(std::move(other.ray));
                    break;
                case SPHERE:
                    new(&sphere) Sphere(std::move(other.sphere));
                    break;
                case PLANE:
                    new(&plane) Plane(std::move(other.plane));
                    break;
            }
            other.isColliding = false;
        }
        return *this;
    }

    ~CollisionShape() {
        switch (shapeType) {
            case RAY:
                ray.~Ray();
                break;
            case SPHERE:
                sphere.~Sphere();
                break;
            case PLANE:
                plane.~Plane();
                break;
        }
    }

    static OverlapingShape intersectionExist(CollisionShape &shapeA, Transform &transformA, CollisionShape &shapeB, Transform &transformB);

    static bool canSee(CollisionShape &checker, CollisionShape &checked);
};
