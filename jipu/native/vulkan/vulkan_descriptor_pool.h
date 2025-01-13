#pragma once

#include "vulkan_api.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace jipu
{

class VulkanDevice;
class VulkanBindGroupLayout;
class VulkanDescriptorPool final
{
public:
    VulkanDescriptorPool() = delete;
    VulkanDescriptorPool(VulkanDevice* device);
    ~VulkanDescriptorPool();

    VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

public:
    VkDescriptorSet allocate(VulkanBindGroupLayout* vulkanBindGroupLayout);
    void free(VkDescriptorSet descriptorSet);

private:
    VkDescriptorPool createDescriptorPool(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

private:
    VulkanDevice* m_device = nullptr;

private:
    std::unordered_map<VkDescriptorPool, VkDescriptorSet> m_descriptorSets{};
};

} // namespace jipu