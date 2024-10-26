#include "vulkan_fence_pool.h"

#include "vulkan_device.h"

#include <spdlog/spdlog.h>

namespace jipu
{

VulkanFencePool::VulkanFencePool(VulkanDevice* device)
    : m_device(device)
{
}

VulkanFencePool::~VulkanFencePool()
{
    for (auto& fence : m_fences)
    {
        if (fence.second)
        {
            spdlog::warn("Fence is not released in this fence pool.");
        }

        m_device->vkAPI.DestroyFence(m_device->getVkDevice(), fence.first, nullptr);
    }
}

VkFence VulkanFencePool::create()
{
    for (auto& fence : m_fences)
    {
        if (fence.second == false)
        {
            fence.second = true;

            if (m_device->vkAPI.ResetFences(m_device->getVkDevice(), 1, &fence.first) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to reset fence.");
            }

            return fence.first;
        }
    }

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = 0;
    // fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence fence = VK_NULL_HANDLE;
    if (m_device->vkAPI.CreateFence(m_device->getVkDevice(), &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create fence.");
    }

    m_fences.insert(std::make_pair(fence, true));

    return fence;
}

void VulkanFencePool::release(VkFence fence)
{
    if (!m_fences.contains(fence))
    {
        spdlog::error("thre fence was not created in this fence pool.");
        return;
    }

    m_fences[fence] = false;
}

} // namespace jipu