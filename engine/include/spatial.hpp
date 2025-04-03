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


namespace spatial {
    class SpatialNode{
        protected:
        SpatialNode *parent_ = nullptr;
        std::vector<SpatialNode*> children_;

        public:
        // SceneNode part
        SpatialNode() = default;
        SpatialNode(const SpatialNode&) = delete;
        SpatialNode(SpatialNode&&) = default;
        virtual ~SpatialNode() = default;
        void SetParent(SpatialNode *parent) {this->parent_ = parent;}
        SpatialNode *GetParent() const {return this->parent_;}
        void AddChild(SpatialNode *child);
        void RemoveChild(SpatialNode* component);
        
        bool IsLeaf() const {return children_.empty();}

        // SpatialNodePart
        Transform* transform;
        void updateSelfAndChildTransform();
        void forceUpdateSelfAndChild();
    };

    class MeshInstance : public SpatialNode{
        protected:
        vector<unsigned short> indices;
        vector<glm::vec3> indexed_vertices;
        vector<glm::vec2> tex_coords;
        void generateBuffer();
        
        public:

        MeshInstance();
        MeshInstance(vector<unsigned short> &indices, vector<glm::vec3> &indexed_vertices, vector<glm::vec2> &tex_coords);
        
        void getBufferData(std::vector<float> &vertex_buffer_data);
        int getNumberOfIndices();
        void* getIndiceIdx();
    };
}
