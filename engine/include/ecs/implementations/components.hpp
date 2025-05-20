#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <functional>
#include <iostream>
#include <unordered_set>
#include <engine/include/ecs/base/entity.hpp>
#include <imgui.h>

template<typename T>
class ComponentInspector;

struct Component{
    Component(const Component&) = delete;
    Component() {}
};


struct CameraComponent: Component {
    bool activated = false;
    bool needActivation = false;
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
    glm::quat rot = {1.0f, 0.f, 0.f, 0.f};
    // glm::vec3 eulerRot = { 0.0f, 0.0f, 0.0f };
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    glm::mat4 modelMatrix = glm::mat4(1.f);
    bool dirty = true;

    glm::mat4 getLocalModelMatrix();

    public:
    
    void computeModelMatrix();
    
    void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix);

    bool isDirty();
    
    void setLocalPosition(glm::vec3 position);
    glm::vec3 getLocalPosition();
    glm::vec3 getGlobalPosition();
    
    glm::quat getLocalRotation();
    void setLocalRotation(glm::vec3 rotationAngles);
    void setLocalRotation(glm::quat rotationQuat);

    glm::vec3 applyRotation(glm::vec3 vector);

    void rotate(glm::vec3 rotations);
    void translate(glm::vec3);

    glm::mat4 getModelMatrix();


    Transform()
    : modelMatrix(1.f), pos(0.0f), scale(1.0f), rot(1.0f, 0.0f, 0.0f, 0.0f), dirty(true){}

    Transform(Transform&& other) noexcept
        : pos(std::move(other.pos)), 
          rot(std::move(other.rot)),
          scale(std::move(other.scale)),
          modelMatrix(std::move(other.modelMatrix)),
          dirty(other.dirty)
    {
        other.dirty = true;
    }

    Transform& operator=(Transform&& other) noexcept {
        if (this != &other) {
            pos = std::move(other.pos);
            rot = std::move(other.rot);
            scale = std::move(other.scale);
            modelMatrix = std::move(other.modelMatrix);
            dirty = other.dirty;

            other.dirty = true;
        }
        return *this;
    }

    void updateInterface(){
        if(ImGui::DragFloat3("Position", &pos[0])) dirty = true;
        if(ImGui::DragFloat4("Rotation", &rot[0])) dirty = true;
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

// Store any value (usefull for customBehavior lambdas)
struct CustomVar: Component {
    CustomVar() = default;
    std::vector<bool> bools;
};


////////////  Physic

struct RigidBody: Component {
    enum BodyTypeEnum {
        RIGID,
        STATIC,
        KINEMATIC
    };
    
    bool dirty = true;
    BodyTypeEnum type = RIGID;
    bool grounded = false;

    
    glm::vec3  torque = glm::vec3(0);
    
    glm::vec3 forces=glm::vec3(0);
    glm::vec3 velocity=glm::vec3(0);
    glm::vec3 gravityDirection = {0,-1,0};
    glm::vec3 angularVelocity=glm::vec3(0);

    glm::mat3 invInertia = glm::mat3(0.4);
    
    float mass=1.f;
    float invMass = 1.f;
    float restitutionCoef=0.5f;
    float frictionCoef=0.6f;


    RigidBody() = default;

    RigidBody(RigidBody&& other) noexcept
        : type(other.type),
          grounded(other.grounded),
          velocity(std::move(other.velocity)),
          mass(other.mass),
          restitutionCoef(other.restitutionCoef),
          frictionCoef(other.frictionCoef)
    {
        other.mass = 1.f; // Ou autre valeur par défaut
        other.restitutionCoef = 0.5f;
        other.frictionCoef = 0.5f;
        other.type = RIGID;
        other.grounded = false;
    }

    RigidBody& operator=(RigidBody&& other) noexcept {
        if (this != &other) {
            type = other.type;
            grounded = other.grounded;
            velocity = std::move(other.velocity);
            mass = other.mass;
            restitutionCoef = other.restitutionCoef;
            frictionCoef = other.frictionCoef;

            other.mass = 1.f;
            other.restitutionCoef = 0.5f;
            other.frictionCoef = 0.5f;
            other.type = RIGID;
            other.grounded = false;
        }
        return *this;
    }

    void setMass(float value){
        mass = value;
        dirty = true;
    }

    void applyForces();
    void addLinearImpulse(const glm::vec3 &impulse);
    void update(float delta);
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
    PLANE,
    AABB,
    OOBB
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

struct Oobb {
    Oobb() = default;
    glm::vec3 halfExtents{1};
};

struct Aabb {
    Aabb() = default;
    glm::vec3 diag{1};
};


struct CollisionShape: Component{
    static uint16_t ENV_LAYER, PLAYER_LAYER, GRAVITY_SENSITIVE_LAYER;
    
    CollisionShapeTypeEnum shapeType;
    union {
        Ray ray;
        Sphere sphere;
        Plane plane;
        Aabb aabb;
        Oobb oobb;
    };
    
    uint16_t layer = 1;
    uint16_t mask = 1;

    std::unordered_set<Entity> collidingEntities;
    bool isColliding(Entity entity) {
        return collidingEntities.find(entity) != collidingEntities.end();
    }
    void addCollision(Entity entity) {
        collidingEntities.insert(entity);
    }
    void removeCollision(Entity entity) {
        collidingEntities.erase(entity);
    }
    bool isAnythingColliding() {
        return !collidingEntities.empty();
    }

    CollisionShape() : shapeType(PLANE) {
        new(&plane) Plane();
    }

    CollisionShape(CollisionShape&& other) noexcept 
        : shapeType(other.shapeType), collidingEntities(other.collidingEntities) {
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
            case AABB:
                new(&aabb) Aabb(std::move(other.aabb));
                break;
            case OOBB:
                new(&oobb) Oobb(std::move(other.oobb));
                break;
        }
        other.collidingEntities.clear();
        layer = other.layer;
        mask = other.mask;
    }

    CollisionShape& operator=(CollisionShape&& other) noexcept {
        if (this != &other) {
            shapeType = other.shapeType;
            collidingEntities = other.collidingEntities;

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
                case AABB:
                    new(&aabb) Aabb(std::move(other.aabb));
                    break;
                case OOBB:
                    new(&oobb) Oobb(std::move(other.oobb));
                    break;
            }
            other.collidingEntities.clear();
            layer = other.layer;
            mask = other.mask;
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
            case AABB:
                aabb.~Aabb();
                break;
            case OOBB:
                oobb.~Oobb();
                break;
        }
    }

    static OverlapingShape intersectionExist(CollisionShape &shapeA, Transform &transformA, CollisionShape &shapeB, Transform &transformB);

    static bool canSee(CollisionShape &checker, CollisionShape &checked);
};
