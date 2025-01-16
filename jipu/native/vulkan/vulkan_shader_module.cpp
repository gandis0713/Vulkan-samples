#include "vulkan_shader_module.h"

#include "jipu/common/hash.h"
#include "vulkan_api.h"
#include "vulkan_device.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

size_t getHash(const VulkanShaderModuleInfo& info)
{
    size_t hash = 0;

    combineHash(hash, info.code);

    return hash;
}

VulkanShaderModule::VulkanShaderModule(VulkanDevice* device, const ShaderModuleDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    m_metaData.info = VulkanShaderModuleInfo{
        .code = std::string(descriptor.code)
    };
    m_metaData.hash = getHash(m_metaData.info);
}

VulkanShaderModule::~VulkanShaderModule()
{
    m_device->getDeleter()->safeDestroy(m_shaderModule);
}

VkShaderModule VulkanShaderModule::getVkShaderModule() const
{
    return m_device->getShaderModuleCache()->getVkShaderModule(m_metaData);
}

const VulkanShaderModuleMetaData& VulkanShaderModule::getMetaData() const
{
    return m_metaData;
}

// VulkanShaderModuleCache

VulkanShaderModuleCache::VulkanShaderModuleCache(VulkanDevice* device)
    : m_device(device)
{
}

VulkanShaderModuleCache::~VulkanShaderModuleCache()
{
    clear();
}

VkShaderModule VulkanShaderModuleCache::getVkShaderModule(const VulkanShaderModuleMetaData& metaData)
{
    auto it = m_shaderModules.find(metaData.hash);
    if (it != m_shaderModules.end())
    {
        return it->second;
    }

    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = metaData.info.code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(metaData.info.code.data());

    auto vulkanDevice = downcast(m_device);

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    auto result = vulkanDevice->vkAPI.CreateShaderModule(vulkanDevice->getVkDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create shader module. [Result: {}]", static_cast<int32_t>(result)));
    }

    m_shaderModules.insert({ getHash(metaData.info), shaderModule });

    return shaderModule;
}

void VulkanShaderModuleCache::clear()
{
    for (auto& [descriptor, shaderModule] : m_shaderModules)
    {
        m_device->getDeleter()->safeDestroy(shaderModule);
    }

    m_shaderModules.clear();
}

} // namespace jipu