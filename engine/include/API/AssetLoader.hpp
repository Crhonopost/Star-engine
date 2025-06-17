#pragma once

#include <string>
#include <vector>

#include <engine/include/rendering/common.hpp>

namespace API {
    namespace AssetLoader {
        bool loadMesh(const std::string &path, std::vector<SingleMesh> &meshes, std::vector<Material> &materials);
    }
}