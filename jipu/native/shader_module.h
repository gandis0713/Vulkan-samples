#pragma once

#include <cstddef>
#include <string_view>

namespace jipu
{

enum class ShaderModuleType
{
    kUndefined = 0,
    kWGSL,
    kSPIRV,
};

struct ShaderModuleDescriptor
{
    ShaderModuleType type = ShaderModuleType::kUndefined;
    std::string_view code;
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