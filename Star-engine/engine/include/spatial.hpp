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

class Program ;
using namespace std;

struct TextureData {
    char *path, *uniformName;
    GLuint id, uniformLocation;
    int activationInt;

    void activate(){
        glActiveTexture(GL_TEXTURE0 + activationInt);
    }
};

struct MeshData {
    GLuint VAO, VBO, EBO;
    int indexCount;
};

namespace spatial {
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

        glm::mat4 getLocalModelMatrix(){
            const glm::mat4 transformX = glm::rotate(glm::mat4(1.0f),
            glm::radians(eulerRot.x),
            glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::mat4 transformY = glm::rotate(glm::mat4(1.0f),
            glm::radians(eulerRot.y),
            glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f),
            glm::radians(eulerRot.z),
            glm::vec3(0.0f, 0.0f, 1.0f));
            
            // Y * X * Z
            glm::mat4 rotationMatrix;
            switch (rotationOrder)
            {
                case YXZ:
                rotationMatrix = transformY * transformX * transformZ;
                break;
                case XYZ:
                rotationMatrix = transformX * transformY * transformZ;
                break;
                case ZYX:
                rotationMatrix = transformZ * transformY * transformX;
                break;
            }
            
            // translation * rotation * scale (also know as TRS matrix)
            return glm::translate(glm::mat4(1.0f), pos) *
            rotationMatrix *
            glm::scale(glm::mat4(1.0f), scale);
        }
        
        public:
        RotationOrderEnum rotationOrder = YXZ;
        
        void computeModelMatrix(){
            modelMatrix = getLocalModelMatrix();
            dirty = false;
        }
        
        void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix){
            modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
            dirty = false;
        }


        bool isDirty(){return dirty;}
        
        void setLocalPosition(glm::vec3 position){
            pos = position;
            dirty = true;
        }
        
        void setLocalRotation(glm::vec3 rotationAngles){
            eulerRot = rotationAngles;
            dirty = true;
        }

        void rotate(glm::vec3 rotations){
            eulerRot += rotations;
            dirty = true;
        }

        glm::mat4 getModelMatrix(){
            return modelMatrix;
        }
    };

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
        Transform transform;
        void updateSelfAndChildTransform();
        void forceUpdateSelfAndChild();
    };

    class MeshInstance : public SpatialNode{
        protected:
        vector<unsigned short> indices;
        vector<glm::vec3> indexed_vertices;
        vector<glm::vec2> tex_coords;
        
        public:
        TextureData texture;
        MeshData meshData;

        MeshInstance();
        MeshInstance(vector<unsigned short> &indices, vector<glm::vec3> &indexed_vertices, vector<glm::vec2> &tex_coords);
        
        void getBufferData(std::vector<float> &vertex_buffer_data);
        int getNumberOfIndices();
        void* getIndiceIdx();
    };

    
    class Sphere : public MeshInstance {
        private:
        float radius;
        void generateMesh();
    
        public:
        Sphere(float radius);
    };
        
    
    class Mountain : public MeshInstance{
        private:
        double width, height;
        glm::vec3 position;
        size_t nbOfVerticesSide;
        float scale;
        
        void generateMesh();

        public:

        Mountain();

        Mountain(double width, double height, glm::vec3 position, size_t nbOfVerticesSide, float scale);

        void addVerticesQuantity(int newNumber);

        int getNumberOfVerticesSide();
    };
}
