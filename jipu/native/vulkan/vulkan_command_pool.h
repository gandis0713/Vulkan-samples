#pragma once

#include "vulkan_api.h"

#include <unordered_map>
#include <unordered_set>

namespace jipu
{

struct VulkanCommandBufferDescriptor
{
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
};

class VulkanDevice;
class VulkanCommandPool final
{

public:
    static std::unique_ptr<VulkanCommandPool> create(VulkanDevice* device);

public:
    VulkanCommandPool() = delete;
    ~VulkanCommandPool();

public:
    VkCommandBuffer create(const VulkanCommandBufferDescriptor& descriptor);
    void release(VkCommandBuffer commandBuffer);

private:
    void createVkCommandPool();

private:
    VulkanDevice* m_device = nullptr;

private:
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

private:
    struct CommandBufferInfo
    {
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bool isUsed = false;
    };
    std::unordered_map<VkCommandBuffer, CommandBufferInfo> m_commandBuffers{};

private:
    explicit VulkanCommandPool(VulkanDevice* device);
};

} // namespace jipu