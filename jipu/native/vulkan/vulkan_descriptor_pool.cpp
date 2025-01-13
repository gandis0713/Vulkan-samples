#include "vulkan_descriptor_pool.h"

#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include <spdlog/spdlog.h>

namespace jipu
{

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice* device)
    : m_device(device)
{
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
    for (auto& pair : m_descriptorSets)
    {
        m_device->vkAPI.FreeDescriptorSets(m_device->getVkDevice(), pair.first, 1, &pair.second);
        m_device->vkAPI.DestroyDescriptorPool(m_device->getVkDevice(), pair.first, nullptr);
    }

    m_descriptorSets.clear();
}

VkDescriptorSet VulkanDescriptorPool::allocate(VulkanBindGroupLayout* vulkanBindGroupLayout)
{
    VkDescriptorPool descriptorPool = createDescriptorPool(vulkanBindGroupLayout->getDescriptorSetLayouts());

    auto descriptorSetLayout = vulkanBindGroupLayout->getVkDescriptorSetLayout();

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkResult result = m_device->vkAPI.AllocateDescriptorSets(m_device->getVkDevice(), &descriptorSetAllocateInfo, &descriptorSet);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets.");
    }

    m_descriptorSets[descriptorPool] = descriptorSet;

    return descriptorSet;
}

void VulkanDescriptorPool::free(VkDescriptorSet descriptorSet)
{
    auto it = std::find_if(m_descriptorSets.begin(), m_descriptorSets.end(), [&](const auto& pair) {
        return pair.second == descriptorSet;
    });

    if (it != m_descriptorSets.end())
    {
        m_device->vkAPI.FreeDescriptorSets(m_device->getVkDevice(), (*it).first, 1, &descriptorSet);
        m_device->vkAPI.DestroyDescriptorPool(m_device->getVkDevice(), (*it).first, nullptr);

        m_descriptorSets.erase(it);
    }
    else
    {
        spdlog::warn("Failed to find descriptor set to free it.");
    }
}

VkDescriptorPool VulkanDescriptorPool::createDescriptorPool(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    const uint32_t maxSets = 32; // TODO: set correct max value.
    const uint64_t descriptorPoolCount = 8;
    const uint64_t maxDescriptorSetSize = descriptorPoolCount;
    std::array<VkDescriptorPoolSize, descriptorPoolCount> poolSizes;
    VkDescriptorPoolCreateInfo poolCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                               .pNext = nullptr,
                                               .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                                               .maxSets = maxSets,
                                               .poolSizeCount = descriptorPoolCount,
                                               .pPoolSizes = poolSizes.data() };

    auto physicalDevice = m_device->getPhysicalDevice();

    const VulkanPhysicalDeviceInfo& physicalDeviceInfo = physicalDevice->getVulkanPhysicalDeviceInfo();
    const VkPhysicalDeviceLimits& devicePropertyLimists = physicalDeviceInfo.physicalDeviceProperties.limits;

    uint32_t kDescriptorSetUniformBufferCount = 32;
    if (devicePropertyLimists.maxDescriptorSetUniformBuffers < kDescriptorSetUniformBufferCount)
        kDescriptorSetUniformBufferCount = devicePropertyLimists.maxDescriptorSetUniformBuffers;

    uint32_t kDescriptorSetUniformBufferDynamicCount = 32;
    if (devicePropertyLimists.maxDescriptorSetUniformBuffersDynamic < kDescriptorSetUniformBufferDynamicCount)
        kDescriptorSetUniformBufferDynamicCount = devicePropertyLimists.maxDescriptorSetUniformBuffersDynamic;

    uint32_t kDescriptorSetSamplers = 32;
    if (devicePropertyLimists.maxDescriptorSetSamplers < kDescriptorSetSamplers)
        kDescriptorSetSamplers = devicePropertyLimists.maxDescriptorSetSamplers;

    uint32_t kDescriptorSetSampledImages = 32;
    if (devicePropertyLimists.maxDescriptorSetSampledImages < kDescriptorSetSampledImages)
        kDescriptorSetSampledImages = devicePropertyLimists.maxDescriptorSetSampledImages;

    uint32_t kDescriptorSetInputAttachments = 32;
    if (devicePropertyLimists.maxDescriptorSetInputAttachments < kDescriptorSetInputAttachments)
        kDescriptorSetInputAttachments = devicePropertyLimists.maxDescriptorSetInputAttachments;

    uint32_t kDescriptorSetStorageBuffers = 32;
    if (devicePropertyLimists.maxDescriptorSetStorageBuffers < kDescriptorSetStorageBuffers)
        kDescriptorSetStorageBuffers = devicePropertyLimists.maxDescriptorSetStorageBuffers;

    uint32_t kDescriptorSetStorageBuffersDynamic = 32;
    if (devicePropertyLimists.maxDescriptorSetStorageBuffersDynamic < kDescriptorSetStorageBuffersDynamic)
        kDescriptorSetStorageBuffersDynamic = devicePropertyLimists.maxDescriptorSetStorageBuffersDynamic;

    poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kDescriptorSetUniformBufferCount };
    poolSizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, kDescriptorSetUniformBufferDynamicCount };
    poolSizes[2] = { VK_DESCRIPTOR_TYPE_SAMPLER, kDescriptorSetSamplers };
    poolSizes[3] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kDescriptorSetSampledImages };
    poolSizes[4] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kDescriptorSetSampledImages };
    poolSizes[5] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, kDescriptorSetInputAttachments };
    poolSizes[6] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kDescriptorSetStorageBuffers };
    poolSizes[7] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, kDescriptorSetStorageBuffersDynamic };

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkResult result = m_device->vkAPI.CreateDescriptorPool(m_device->getVkDevice(), &poolCreateInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create descriptor pool. {}", static_cast<uint32_t>(result)));
    }

    return descriptorPool;
}

} // namespace jipu