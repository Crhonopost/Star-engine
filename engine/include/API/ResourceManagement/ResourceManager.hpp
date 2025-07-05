#pragma once
#include <unordered_map>
#include <string>
#include <memory>

#include <engine/include/API/ResourceManagement/IResource.hpp>
#include <engine/include/rendering/common.hpp>

template<typename T, typename... Args>
class ResourceManager {
    static_assert(std::is_base_of<IResource, T>::value, "T must inherit from IResource");

private:
    std::unordered_map<std::string, std::weak_ptr<T>> resources;
    Loader<T, Args...> loader;

public:
    explicit ResourceManager(Loader<T, Args...> loader) : loader(std::move(loader)) {}
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = default;

    ~ResourceManager() = default;

    std::shared_ptr<T> hasResource(const std::string &name){
        auto it = resources.find(name);
        if (it != resources.end()) {
            auto resource = it->second.lock();
            if (resource) {
                return resource;
            }
        }

        return nullptr;
    }


    std::shared_ptr<T> load(const std::string &name, Args... args) {
        auto resource = hasResource(name);
        if(resource) return resource;
        
        resource = loader.load(args...);
        // TODO: use std::forward
        if (resource) {
            resources[name] = resource;
        }

        return resource;
    }

    // If the resource is loaded in advance and just needs to exist in the manager
    // Only if you know what you are doing (the responsibility of checking if the resource is already loaded is not included here)
    // TODO: find a better solution for internal resources
    std::shared_ptr<T> forceLoad(const std::string &name, const std::shared_ptr<T> &resource) {
        resources[name] = resource;
    }
};

auto internalTextureLoader = Loader<Texture, unsigned char*, size_t, int, int, int, std::string>([](const unsigned char* data,
                                                                                                    size_t size,
                                                                                                    int width,
                                                                                                    int height,
                                                                                                    int channels,
                                                                                                    const std::string& key){
    auto res = std::make_shared<Texture>(data, size, width, height, channels, key);
    return res;
});

auto externalTextureLoader = Loader<Texture, std::string>([](const std::string& path){
    auto res = std::make_shared<Texture>(path);
    return res;
});

auto meshLoader = Loader<MultiMesh, std::string>([](const std::string& path){
    auto res = std::make_shared<MultiMesh>(path);
    return res;
});

namespace Managers{
    ResourceManager<Texture, std::string> externalTextureManager(externalTextureLoader);
    ResourceManager<Texture, unsigned char*, size_t, int, int, int, std::string> internalTextureManager(internalTextureLoader);
    ResourceManager<MultiMesh, std::string> meshManager(meshLoader);
}

