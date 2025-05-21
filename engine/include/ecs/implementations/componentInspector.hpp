#pragma once

#include <common/json.hpp>
#include <engine/include/ecs/ecsWithoutInspector.hpp>

using json = nlohmann::json;

class IComponentInspector {
    public:
    virtual ~IComponentInspector() = default;
    virtual void DisplayGUI(ecsWithoutInspector &ecs, Entity entity) = 0;
    virtual json GetJson(ecsWithoutInspector &ecs, Entity entity) = 0;
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

    inline json GetJson(ecsWithoutInspector &ecs, Entity entity) override {
        json res;
        if (ecs.HasComponent<T>(entity)) {
            auto& component = ecs.GetComponent<T>(entity);
            res = GetComponentJson(component);
        }
        return res;
    }
    static json GetComponentJson(T& component);
};



// Imgui
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
inline void ComponentInspector<CameraComponent>::DisplayComponentGUI(CameraComponent& cameraComponent) {}

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


template<>
inline void ComponentInspector<Transform>::DisplayComponentGUI(Transform& transform) {
    ImGui::SeparatorText("Transform");
    if(ImGui::DragFloat3("Position", &transform.pos.x)) transform.dirty = true;
    
    // glm::vec3 rotationEuler = transform.getLocalRotation();
    // if(ImGui::DragFloat3("Rotation", &rotationEuler.x)){
    //     transform.dirty = true;
    //     transform.setLocalRotation(rotationEuler);
    // } 

    if(ImGui::DragFloat3("Scale", &transform.scale.x)) transform.dirty = true;
}

template<>
inline void ComponentInspector<CustomBehavior>::DisplayComponentGUI(CustomBehavior& customBehavior) {}
template<>
inline void ComponentInspector<CustomVar>::DisplayComponentGUI(CustomVar& var){}

template<>
inline void ComponentInspector<RigidBody>::DisplayComponentGUI(RigidBody& rigidBody) {
    ImGui::SeparatorText("Rigid body");
    if(ImGui::BeginMenu("Body type")){
        if(ImGui::MenuItem("Rigid")) rigidBody.type = RigidBody::RIGID;
        if(ImGui::MenuItem("Static")) rigidBody.type = RigidBody::STATIC;
        if(ImGui::MenuItem("Kinematic")) rigidBody.type = RigidBody::KINEMATIC;
        ImGui::EndMenu();
    }
    // TODO: chaging shape doesn't force rigid body invert inertia to be recalculated
    if(ImGui::DragFloat("Mass", &rigidBody.mass)) rigidBody.dirty = true;
    ImGui::DragFloat("Friction coef", &rigidBody.frictionCoef);
    ImGui::DragFloat("Restitution coef", &rigidBody.restitutionCoef);
    ImGui::DragFloat3("Velocity", &rigidBody.velocity[0]);
    ImGui::DragFloat3("Angular velocity", &rigidBody.angularVelocity[0]);

}

template<>
inline void ComponentInspector<CollisionShape>::DisplayComponentGUI(CollisionShape& collisionShape) {
    ImGui::SeparatorText("Collision shape");
    if(ImGui::BeginMenu("Type")){
        if(ImGui::MenuItem("Sphere")) collisionShape.shapeType = SPHERE;
        if(ImGui::MenuItem("Ray")) collisionShape.shapeType = RAY;
        if(ImGui::MenuItem("Plane")) collisionShape.shapeType = PLANE;
        if(ImGui::MenuItem("AABB")) collisionShape.shapeType = AABB;
        if(ImGui::MenuItem("OOBB")) collisionShape.shapeType = OOBB;
        ImGui::EndMenu();
    }

    if(collisionShape.shapeType == RAY){
        ImGui::DragFloat("Length", &collisionShape.ray.length);
    } else if(collisionShape.shapeType == SPHERE){
        ImGui::DragFloat("Radius", &collisionShape.sphere.radius);
    } else if(collisionShape.shapeType == AABB){
        ImGui::DragFloat3("Half", &collisionShape.aabb.diag[0]);
    } else if(collisionShape.shapeType == OOBB){
        ImGui::DragFloat3("Half", &collisionShape.oobb.halfExtents[0], 0.2f, 0.01f);
    }

}


// Json serialization
// json serializeVec3(glm::vec3 &vec){
//     return {
//         {"x", vec.x},
//         {"y", vec.y},
//         {"z", vec.z}
//     };
// }

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
inline json ComponentInspector<CameraComponent>::GetComponentJson(CameraComponent& comp){
    return {{"name", "CameraComponent"}};
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

template<>
inline json ComponentInspector<Transform>::GetComponentJson(Transform& transform){
    return {{"name", "Transform"}};
}
template<>
inline json ComponentInspector<CustomBehavior>::GetComponentJson(CustomBehavior& custom){
    return {{"name", "CustomBehavior"}};
}
template<>
inline json ComponentInspector<CustomVar>::GetComponentJson(CustomVar& custom){
    return {{"name", "CustomVar"}};
}
template<>
inline json ComponentInspector<RigidBody>::GetComponentJson(RigidBody& rigidBody){
    return {{"name", "RigidBody"}};
}
template<>
inline json ComponentInspector<CollisionShape>::GetComponentJson(CollisionShape& collisionShape){
    return {{"name", "CollisionShape"}};
}


