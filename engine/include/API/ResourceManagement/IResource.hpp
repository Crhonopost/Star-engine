#pragma once
#include <string>

struct IResource {
    virtual ~IResource() = default;
    virtual bool load(const std::string &name) = 0;
};
