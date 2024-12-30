#include "vulkan_command_pool.h"

#include "vulkan_device.h"

#include <spdlog/spdlog.h>

namespace jipu
{

std::unique_ptr<VulkanCommandPool> VulkanCommandPool::create(VulkanDevice* device)
{
    auto vulkanCommandPool = std::unique_ptr<VulkanCommandPool>(new VulkanCommandPool(device));
    vulkanCommandPool->createVkCommandPool();

    return vulkanCommandPool;
}

VulkanCommandPool::VulkanCommandPool(VulkanDevice* device)
    : m_device(device)
{
}

VulkanCommandPool::~VulkanCommandPool()
{
    for (auto& [commandBuffer, info] : m_commandBuffers)
    {
        if (info.isUsed)
        {
            spdlog::warn("Command buffer is not released in this command buffer pool.");
        }

        m_device->vkAPI.FreeCommandBuffers(m_device->getVkDevice(), m_commandPool, 1, &commandBuffer);
    }

    m_device->vkAPI.DestroyCommandPool(m_device->getVkDevice(), m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;
}

VkCommandBuffer VulkanCommandPool::create(const VulkanCommandBufferDescriptor& descriptor)
{
    for (auto& [commandBuffer, info] : m_commandBuffers)
    {
        if (info.isUsed == false && info.level == descriptor.level)
        {
            info.isUsed = true;
            return commandBuffer;
        }
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = m_commandPool;
    commandBufferAllocateInfo.level = descriptor.level;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (m_device->vkAPI.AllocateCommandBuffers(m_device->getVkDevice(), &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command buffer.");
    }

    m_commandBuffers.insert(std::make_pair(commandBuffer, CommandBufferInfo{ .level = descriptor.level, .isUsed = true }));

    return commandBuffer;
}

void VulkanCommandPool::release(VkCommandBuffer commandBuffer)
{
    if (!m_commandBuffers.contains(commandBuffer))
    {
        spdlog::error("The command buffer was not created in this command buffer pool.");
        return;
    }

    m_commandBuffers[commandBuffer].isUsed = false;
}

void VulkanCommandPool::createVkCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = 0; // TODO: queue family index.
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (m_device->vkAPI.CreateCommandPool(m_device->getVkDevice(), &commandPoolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool.");
    }
}

} // namespace jipu