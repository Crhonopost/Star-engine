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
            res.push_back(GetComponentJson(component));
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


// Json serialization
template<>
inline json ComponentInspector<Drawable>::GetComponentJson(Drawable& drawable){
    return {
        {
            "Drawable", {{"switch_distance", drawable.switchDistance}}
        }
    };
}

template<>
inline json ComponentInspector<Transform>::GetComponentJson(Transform& transform){
    return {"Transform", false};
}
template<>
inline json ComponentInspector<CustomBehavior>::GetComponentJson(CustomBehavior& custom){
    return {"CustomBehavior", false};
}
template<>
inline json ComponentInspector<RigidBody>::GetComponentJson(RigidBody& rigidBody){
    return {"RigidBody", false};
}
template<>
inline json ComponentInspector<CollisionShape>::GetComponentJson(CollisionShape& collisionShape){
    return {"CollisionShape", false};
}


