#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <functional>
#include <iostream>
#include <engine/include/ecs/base/entity.hpp>
#include <imgui.h>
#include <engine/include/ecs/ecsWithoutInspector.hpp>


class IComponentInspector {
    public:
    virtual ~IComponentInspector() = default;
    virtual void DisplayGUI(ecsWithoutInspector &ecs, Entity entity) = 0;
    virtual const char* GetName() = 0;
};

template<typename T>
class ComponentInspector : public IComponentInspector {
public:
    const char* GetName() override {
        return typeid(T).name();
    }

    inline void DisplayGUI(ecsWithoutInspector &ecs, Entity entity) override {
        if (ecs.HasComponent<T>(entity)) {
            auto& component = ecs.GetComponent<T>(entity);
            DisplayComponentGUI(component);
        }
    }
    
    static void DisplayComponentGUI(T& component);
};



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
    friend class ComponentInspector<Transform>;

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

    void updateInterface(){
        if(ImGui::DragFloat3("Position", &pos[0])) dirty = true;
        if(ImGui::DragFloat3("Rotation", &eulerRot[0])) dirty = true;
        if(ImGui::BeginMenu("Rotation order")){
            if(ImGui::MenuItem("XYZ")) rotationOrder = XYZ;
            if(ImGui::MenuItem("YXZ")) rotationOrder = YXZ;
            if(ImGui::MenuItem("ZYX")) rotationOrder = ZYX;
            ImGui::EndMenu();
        }
        if(ImGui::DragFloat3("Scale", &scale[0])) dirty = true;
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


// Inspectors

template<>
inline void ComponentInspector<Drawable>::DisplayComponentGUI(Drawable& drawable){
    ImGui::SeparatorText("Drawable");
    ImGui::DragFloat("Lod switch distance", &drawable.switchDistance);
}

template<>
inline void ComponentInspector<Transform>::DisplayComponentGUI(Transform& transform) {
    ImGui::SeparatorText("Transform");
    if(ImGui::DragFloat3("Position", &transform.pos.x)) transform.dirty = true;
    if(ImGui::DragFloat3("Rotation", &transform.eulerRot.x)) transform.dirty = true;
    if(ImGui::DragFloat3("Scale", &transform.scale.x)) transform.dirty = true;
}

template<>
inline void ComponentInspector<CustomBehavior>::DisplayComponentGUI(CustomBehavior& customBehavior) {}

template<>
inline void ComponentInspector<RigidBody>::DisplayComponentGUI(RigidBody& rigidBody) {
    ImGui::SeparatorText("Rigid body");
    ImGui::DragFloat("Weight", &rigidBody.weight);
    ImGui::DragFloat("Friction coef", &rigidBody.frictionCoef);
    ImGui::DragFloat("Restitution coef", &rigidBody.restitutionCoef);
    ImGui::Checkbox("Static", &rigidBody.isStatic);
}

template<>
inline void ComponentInspector<CollisionShape>::DisplayComponentGUI(CollisionShape& collisionShape) {
}




