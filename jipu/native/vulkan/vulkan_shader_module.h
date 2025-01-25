#pragma once

#include "shader_module.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include "vulkan_pipeline_layout.h"

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

class VulkanDevice;
class VULKAN_EXPORT VulkanShaderModule : public ShaderModule
{
public:
    VulkanShaderModule() = delete;
    VulkanShaderModule(VulkanDevice* device, const ShaderModuleDescriptor& descriptor);
    ~VulkanShaderModule() override;

    VkShaderModule getVkShaderModule(const VulkanPipelineLayoutInfo& layoutInfo,
                                     const std::string_view entryPoint) const;
    const VulkanShaderModuleInfo& getInfo() const;

private:
    VulkanDevice* m_device = nullptr;

private:
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    const ShaderModuleDescriptor m_descriptor{};
    VulkanShaderModuleInfo m_info{};
};
DOWN_CAST(VulkanShaderModule, ShaderModule);

struct VulkanShaderModuleMetaData
{
    VulkanShaderModuleInfo modulInfo{};
    VulkanPipelineLayoutInfo layoutInfo{};
    std::string entryPoint;
};

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
    VkShaderModule createWGSLShaderModule(const VulkanShaderModuleMetaData& metaData);
    VkShaderModule createSPIRVShaderModule(const VulkanShaderModuleMetaData& metaData);

private:
    VulkanDevice* m_device = nullptr;

private:
    struct Functor
    {
        size_t operator()(const VulkanShaderModuleMetaData& metaData) const;
        bool operator()(const VulkanShaderModuleMetaData& lhs, const VulkanShaderModuleMetaData& rhs) const;
    };
    using Cache = std::unordered_map<VulkanShaderModuleMetaData, VkShaderModule, Functor, Functor>;
    Cache m_shaderModuleCache{};
};

} // namespace jipu
