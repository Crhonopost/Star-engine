#pragma once
#include <unordered_map>
#include <string>
#include <memory>

#include <engine/include/API/ResourceManagement/IResource.hpp>
#include <engine/include/rendering/common.hpp>

template<typename T>
class ResourceManager {
    static_assert(std::is_base_of<IResource, T>::value, "T must inherit from IResource");

private:
    std::unordered_map<std::string, std::weak_ptr<T>> resources;

public:
    ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = default;

    ~ResourceManager() = default;


    static std::shared_ptr<T> load(const std::string &name, bool isInternal = false) {
        auto &resources = instance().resources;
        
        auto it = resources.find(name);
        if (it != resources.end()) {
            auto resource = it->second.lock();
            if (resource) {
                return resource;
            }
        } else if (isInternal) {
            return nullptr;
        }
        
        auto resource = std::make_shared<T>();
        if (resource->load(name)) {
            resources[name] = resource;
            return resource;
        } else {
            return nullptr; // or handle error
        }
    }

    // If the resource is loaded in advance and just needs to exist in the manager
    static std::shared_ptr<T> addPreloaded(const std::string &name, const std::shared_ptr<T> &resource) {
        auto &resources = instance().resources;

        auto it = resources.find(name);
        if (it != resources.end()) {
            auto resource = it->second.lock();
            if (resource) {
                return resource;
            }
        }
        
        resources[name] = resource;
    }

    static ResourceManager<T>& instance() {
        static ResourceManager<T> instance;
        return instance;
    }
};

typedef ResourceManager<Texture> TextureManager;
typedef ResourceManager<Material> MaterialManager;

typedef ResourceManager<MultiMesh> MultiMeshManager;
typedef ResourceManager<SingleMesh> SingleMeshManager;
