#include <engine/include/animation.hpp>
#include <iostream>
#include <glm/gtx/string_cast.hpp>  // Pour glm::to_string
#include <glm/gtx/matrix_decompose.hpp>  // Pour décomposer les matrices





BoneTransform  BoneAnimation::interpolate(float time) const {
    BoneTransform result;

    auto interpVec3 = [](const auto& keys, float time) {
        if (keys.empty()) return glm::vec3(0.0f);
        if (time <= keys.front().first) return keys.front().second;
        if (time >= keys.back().first) return keys.back().second;

        for (size_t i = 0; i + 1 < keys.size(); ++i) {
            float t0 = keys[i].first, t1 = keys[i + 1].first;
            if (time >= t0 && time <= t1) {
                float f = (time - t0) / (t1 - t0);
                return glm::mix(keys[i].second, keys[i + 1].second, f);
            }
        }
        return keys.back().second;
    };

    auto interpQuat = [](const auto& keys, float time) {
        if (keys.empty()) return glm::quat(1, 0, 0, 0);
        if (time <= keys.front().first) return keys.front().second;
        if (time >= keys.back().first) return keys.back().second;

        for (size_t i = 0; i + 1 < keys.size(); ++i) {
            float t0 = keys[i].first, t1 = keys[i + 1].first;
            if (time >= t0 && time <= t1) {
                float f = (time - t0) / (t1 - t0);
                glm::quat a = keys[i].second;
                glm::quat b = keys[i+1].second;
                if (glm::dot(a,b) < 0.0f) b = -b;
                return glm::slerp(a, b, f);
            }
        }
        return keys.back().second;
    };

    result.position = interpVec3(positionKeys, time);
    result.rotation = interpQuat(rotationKeys, time);
    result.scale    = interpVec3(scaleKeys, time);

    return result;
}


void Animation::addDeltaTime(float delta){
    currentTime += delta;
    if(loop && currentTime > duration/1000.f){
        currentTime -= duration/1000.f;
    }
}

void Animation::getPose(std::vector<Bone>& bones, std::vector<glm::mat4>& outMatrices) const {
    outMatrices.resize(bones.size(), glm::mat4(1.0f));

    for (size_t i = 0; i < bones.size(); ++i) {
        const std::string& boneName = bones[i].name;
        auto it = boneAnimations.find(boneName);
        if (it != boneAnimations.end()) {
            BoneTransform tr = it->second.interpolate(currentTime * ticksPerSecond);
            outMatrices[i] = tr.toMat4();
        }
    }

}

void PrintBoneState(const std::string& label, const glm::mat4& matrix) {
    // Décompose la matrice en composantes
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, scale, rotation, translation, skew, perspective);

    std::cout << "----- " << label << " -----\n";
    std::cout << "Matrice complète:\n" << glm::to_string(matrix) << "\n";
    std::cout << "Translation: " << glm::to_string(translation) << "\n";
    std::cout << "Rotation: " << glm::to_string(glm::eulerAngles(rotation)) << " (radians)\n";
    std::cout << "Scale: " << glm::to_string(scale) << "\n\n";
}

void CalculateAnimationPose(const std::vector<Bone>& bones,
    const std::vector<glm::mat4>& animationTransforms,
    std::vector<glm::mat4>& finalMatrices)
{
    finalMatrices.resize(bones.size());
    std::vector<glm::mat4> localTransforms(bones.size());
    std::vector<glm::mat4> modelMatrices(bones.size());

    for(int i = 0; i < bones.size(); i++) {
        localTransforms[i] = bones[i].localTransform * animationTransforms[i];
        // localTransforms[i] = animationTransforms[i];  
    }

    modelMatrices[0] = localTransforms[0];

    for(size_t i = 1; i < bones.size(); i++) {
        modelMatrices[i] = modelMatrices[bones[i].parentIdx] * localTransforms[i];
    }

    for(int i = 0; i < bones.size(); i++) {
        finalMatrices[i] = modelMatrices[i] * bones[i].offsetMatrix;
    }
}

glm::mat4 convertToGlm(const aiMatrix4x4& m) {
    return glm::transpose(glm::mat4(
        m.a1, m.a2, m.a3, m.a4,
        m.b1, m.b2, m.b3, m.b4,
        m.c1, m.c2, m.c3, m.c4,
        m.d1, m.d2, m.d3, m.d4
    ));
}
