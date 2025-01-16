#pragma once

#include "shader_module.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include "jipu/common/cast.h"

#include <string>
#include <unordered_map>

namespace jipu
{

struct VulkanShaderModuleInfo
{
    ShaderModuleType type = ShaderModuleType::kUndefined;
    std::string code;
};

struct VulkanShaderModuleMetaData
{
    VulkanShaderModuleInfo info{};
    size_t hash = 0;
};

class VulkanDevice;
class VULKAN_EXPORT VulkanShaderModule : public ShaderModule
{
public:
    VulkanShaderModule() = delete;
    VulkanShaderModule(VulkanDevice* device, const ShaderModuleDescriptor& descriptor);
    ~VulkanShaderModule() override;

    VkShaderModule getVkShaderModule(const std::string_view entryPoint) const;
    const VulkanShaderModuleMetaData& getMetaData() const;

private:
    VulkanDevice* m_device = nullptr;

private:
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    const ShaderModuleDescriptor m_descriptor{};
    VulkanShaderModuleMetaData m_metaData{};
};
DOWN_CAST(VulkanShaderModule, ShaderModule);

class VulkanShaderModuleCache
{
public:
    VulkanShaderModuleCache() = delete;
    VulkanShaderModuleCache(VulkanDevice* device);
    ~VulkanShaderModuleCache();

public:
    VkShaderModule getVkShaderModule(const VulkanShaderModuleMetaData& metaData, std::string_view entryPoint);
    void clear();

private:
    VkShaderModule createWGSLShaderModule(const VulkanShaderModuleMetaData& metaData, std::string_view entryPoint);
    VkShaderModule createSPIRVShaderModule(const VulkanShaderModuleMetaData& metaData);

private:
    VulkanDevice* m_device = nullptr;

private:
    using Cache = std::unordered_map<size_t, std::unordered_map<std::string_view, VkShaderModule>>;
    Cache m_shaderModuleCache{};
};

} // namespace jipu
