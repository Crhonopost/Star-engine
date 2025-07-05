#pragma once
#include <string>
#include <functional>

struct IResource {
    virtual ~IResource() = default;
};

template<typename IResource, typename... Args>
struct Loader {
    using LoaderFunc = std::function<std::shared_ptr<IResource>(Args...)>;
    LoaderFunc load;

    Loader(LoaderFunc l) : load(std::move(l)) {}
};
