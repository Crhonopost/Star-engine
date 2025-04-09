#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <bits/algorithmfwd.h>
#include <iostream>

#include <engine/include/camera.hpp>
#include <engine/include/ecs/implementations/components.hpp>

using namespace std;


class SpatialNode {
protected:
    SpatialNode* parent_ = nullptr;
    std::vector<std::unique_ptr<SpatialNode>> children_;  // Changement ici pour unique_ptr

public:
    // SceneNode part
    SpatialNode() = default;
    SpatialNode(const SpatialNode&) = delete;  // Interdit la copie
    SpatialNode(SpatialNode&&) = default;  // Autorise le déplacement
    SpatialNode(Transform* transform): transform(transform){};
    
    virtual ~SpatialNode() = default;

    void SetParent(SpatialNode* parent) {
        this->parent_ = parent;
    }

    SpatialNode* GetParent() const {
        return this->parent_;
    }

    void AddChild(std::unique_ptr<SpatialNode> child);  // Prend un unique_ptr
    void RemoveChild(SpatialNode* component);
    
    bool IsLeaf() const {
        return children_.empty();
    }

    // SpatialNodePart
    Transform* transform;
    void updateSelfAndChildTransform();
    void forceUpdateSelfAndChild();
};
