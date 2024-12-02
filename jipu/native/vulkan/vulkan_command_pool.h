#pragma once

#include "vulkan_api.h"

#include <unordered_map>
#include <unordered_set>

namespace jipu
{

class VulkanDevice;
class VulkanCommandPool final
{

public:
    static std::unique_ptr<VulkanCommandPool> create(VulkanDevice* device);

public:
    VulkanCommandPool() = delete;
    ~VulkanCommandPool();

public:
    VkCommandBuffer create(/* TODO */);
    void release(VkCommandBuffer commandBuffer);

private:
    void createVkCommandPool();

private:
    VulkanDevice* m_device = nullptr;

private:
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

private:
    std::unordered_map<VkCommandBuffer, bool> m_commandBuffers{};

private:
    std::unordered_set<VkCommandBuffer> m_releasePendingCommandBuffers{};

private:
    explicit VulkanCommandPool(VulkanDevice* device);
};

} // namespace jipu