#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <assimp/matrix4x4.h>

// --- Structures de base ---

struct Bone {
    int parentIdx;  // Index du parent (-1 pour la racine)
    std::string name;        // Nom de l'os
    
    // Transformations :
    glm::mat4 offsetMatrix;
    glm::mat4 localTransform;
};

struct BoneTransform {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 toMat4() const {
        glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 R = glm::toMat4(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        return T * R * S;
    }
};

struct BoneAnimation {
    std::string nodeName;
    std::vector<std::pair<float, glm::vec3>> positionKeys;
    std::vector<std::pair<float, glm::quat>> rotationKeys;
    std::vector<std::pair<float, glm::vec3>> scaleKeys;

    BoneTransform interpolate(float time) const;
};

struct Animation {
    float duration = 0.f;
    float ticksPerSecond = 25.f;
    float currentTime = 0.f;
    bool loop = true;
    std::unordered_map<std::string, int> boneNameToIdx;
    std::unordered_map<std::string, BoneAnimation> boneAnimations;

    void resetDuration(){duration = 0.0f;}
    void addDeltaTime(float value);

    void getPose(std::vector<Bone>& bones, std::vector<glm::mat4>& outInMatrices) const;
};


void CalculateAnimationPose(const std::vector<Bone>& bones,
    const std::vector<glm::mat4>& animationTransforms,
    std::vector<glm::mat4>& finalMatrices);

glm::mat4 convertToGlm(const aiMatrix4x4& m);
