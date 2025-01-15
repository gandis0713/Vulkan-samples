#pragma once

#include "shader_module.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include "jipu/common/cast.h"

#include <string>

namespace jipu
{

struct VulkanShaderModuleInfo
{
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

    VkShaderModule getVkShaderModule() const;
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
    VkShaderModule getVkShaderModule(const VulkanShaderModuleMetaData& metaData);
    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    using Cache = std::unordered_map<size_t, VkShaderModule>;
    Cache m_shaderModules{};
};

} // namespace jipu
