#pragma once

#include <cstddef>

namespace jipu
{

struct ShaderModuleDescriptor
{
    // TODO: use std::string_view
    const char* code = nullptr;
    size_t codeSize = 0;
};

class ShaderModule
{
public:
    virtual ~ShaderModule() = default;

    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

protected:
    ShaderModule() = default;
};

} // namespace jipu