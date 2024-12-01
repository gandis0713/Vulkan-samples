#include "vulkan_shader_module.h"
#include "vulkan_api.h"
#include "vulkan_device.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

VulkanShaderModule::VulkanShaderModule(VulkanDevice* device, const ShaderModuleDescriptor& descriptor)
    : m_device(device)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = descriptor.codeSize;
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(descriptor.code);

    auto vulkanDevice = downcast(m_device);
    auto result = vulkanDevice->vkAPI.CreateShaderModule(vulkanDevice->getVkDevice(), &shaderModuleCreateInfo, nullptr, &m_shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create shader module. [Result: {}]", static_cast<int32_t>(result)));
    }
}

VulkanShaderModule::~VulkanShaderModule()
{
    m_device->getDeleter()->safeDestroy(m_shaderModule);
}

VkShaderModule VulkanShaderModule::getVkShaderModule() const
{
    return m_shaderModule;
}

} // namespace jipu