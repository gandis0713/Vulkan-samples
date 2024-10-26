#pragma once

#include "shader_module.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include "jipu/common/cast.h"

namespace jipu
{

class VulkanDevice;
class VULKAN_EXPORT VulkanShaderModule : public ShaderModule
{
public:
    VulkanShaderModule() = delete;
    VulkanShaderModule(VulkanDevice& device, const ShaderModuleDescriptor& descriptor);
    ~VulkanShaderModule() override;

    VkShaderModule getVkShaderModule() const;

private:
    VulkanDevice& m_device;

private:
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
};

DOWN_CAST(VulkanShaderModule, ShaderModule);

} // namespace jipu
