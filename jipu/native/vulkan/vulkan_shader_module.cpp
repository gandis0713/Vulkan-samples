#include "vulkan_shader_module.h"
#include "vulkan_api.h"
#include "vulkan_device.h"

#include <stdexcept>

namespace jipu
{

VulkanShaderModule::VulkanShaderModule(VulkanDevice& device, const ShaderModuleDescriptor& descriptor)
    : m_device(device)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = descriptor.codeSize;
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(descriptor.code);

    auto& vulkanDevice = downcast(m_device);
    if (vulkanDevice.vkAPI.CreateShaderModule(vulkanDevice.getVkDevice(), &shaderModuleCreateInfo, nullptr, &m_shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module");
    }
}

VulkanShaderModule::~VulkanShaderModule()
{
    auto& vulkanDevice = downcast(m_device);
    vulkanDevice.vkAPI.DestroyShaderModule(vulkanDevice.getVkDevice(), m_shaderModule, nullptr);
}

VkShaderModule VulkanShaderModule::getVkShaderModule() const
{
    return m_shaderModule;
}

} // namespace jipu