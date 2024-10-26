#pragma once

#include <cstddef>

namespace jipu
{

struct ShaderModuleDescriptor
{
    const char* code = nullptr;
    size_t codeSize = 0;
};

class ShaderModule
{
public:
    virtual ~ShaderModule() = default;

protected:
    ShaderModule() = default;
};

} // namespace jipu