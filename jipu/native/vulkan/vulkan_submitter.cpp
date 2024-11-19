#include "vulkan_submitter.h"

#include "vulkan_device.h"

namespace jipu
{

VulkanSubmitter::VulkanSubmitter(VulkanDevice* device)
    : m_device(device)
{
    const auto& queueFamilies = m_device->getActivatedQueueFamilies();
    if (queueFamilies.size() <= 0)
    {
        throw std::runtime_error("There is no activated queue familys.");
    }

    // collect all queues.
    for (auto queueFamilyIndex = 0; queueFamilyIndex < queueFamilies.size(); ++queueFamilyIndex)
    {
        auto& queueFamily = queueFamilies[queueFamilyIndex];

        QueueGroup queueGroup = { .flags = ToQueueFlags(queueFamily.queueFlags), .queues = {} };
        for (auto queueIndex = 0; queueIndex < queueFamily.queueCount; ++queueIndex)
        {
            VkQueue queue{ VK_NULL_HANDLE };
            m_device->vkAPI.GetDeviceQueue(m_device->getVkDevice(), queueFamilyIndex, queueIndex, &queue);

            queueGroup.queues.push_back(queue);
        }

        m_queueGroups.push_back(queueGroup);
    }
}

VulkanSubmitter::~VulkanSubmitter()
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    // wait idle state before destroy semaphore.
    for (auto& queueGroup : m_queueGroups)
        for (auto queue : queueGroup.queues)
            vkAPI.QueueWaitIdle(queue);

    // Doesn't need to destroy VkQueue.
}

std::future<void> VulkanSubmitter::submitAsync(VkFence fence, const std::vector<VulkanSubmit::Info>& submits)
{
    auto submitInfoSize = submits.size();

    std::vector<VkSubmitInfo> submitInfos{};
    submitInfos.resize(submitInfoSize);

    for (auto i = 0; i < submitInfoSize; ++i)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = static_cast<uint32_t>(submits[i].commandBuffers.size());
        submitInfo.pCommandBuffers = submits[i].commandBuffers.data();
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(submits[i].signalSemaphores.size());
        submitInfo.pSignalSemaphores = submits[i].signalSemaphores.data();
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(submits[i].waitSemaphores.size());
        submitInfo.pWaitSemaphores = submits[i].waitSemaphores.data();
        submitInfo.pWaitDstStageMask = submits[i].waitStages.data();

        submitInfos[i] = submitInfo;
    }

    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    auto queue = getVkQueue(VulkanQueueFlagBits::kAll); // TODO: get by queue flags
    VkResult result = vkAPI.QueueSubmit(queue, static_cast<uint32_t>(submitInfos.size()), submitInfos.data(), fence);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to submit command buffer {}", static_cast<uint32_t>(result)));
    }

    auto submitTask = [this, fence = fence, submits = submits]() -> void {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        auto vulkanDevice = downcast(m_device);
        const VulkanAPI& vkAPI = vulkanDevice->vkAPI;
        VkResult result = vkAPI.WaitForFences(vulkanDevice->getVkDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("failed to wait for fences {}", static_cast<uint32_t>(result)));
        }

        result = vkAPI.ResetFences(vulkanDevice->getVkDevice(), 1, &fence);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("failed to reset for fences {}", static_cast<uint32_t>(result)));
        }
    };

    return m_threadPool.enqueue(submitTask);
}

void VulkanSubmitter::submit(VkFence fence, const std::vector<VulkanSubmit::Info>& submits)
{
    submitAsync(fence, submits).get();

    m_device->getInflightObjects()->clear(fence);
}

void VulkanSubmitter::present(VulkanPresentInfo presentInfo)
{
    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = static_cast<uint32_t>(presentInfo.waitSemaphores.size());
    info.pWaitSemaphores = presentInfo.waitSemaphores.data();
    info.swapchainCount = static_cast<uint32_t>(presentInfo.swapchains.size());
    info.pSwapchains = presentInfo.swapchains.data();
    info.pImageIndices = presentInfo.imageIndices.data();
    info.pResults = nullptr; // Optional

    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    auto queue = getVkQueue(VulkanQueueFlagBits::kAll); // TODO: get by queue flags
    vkAPI.QueuePresentKHR(queue, &info);
}

VkQueue VulkanSubmitter::getVkQueue(VulkanQueueFlags flags) const
{
    for (const auto& queueGroup : m_queueGroups)
    {
        if (queueGroup.flags & flags)
        {
            return queueGroup.queues[0];
        }
    }

    throw std::runtime_error("There is no queue family properties.");
    return VK_NULL_HANDLE;
}

// Convert Helper
VkQueueFlags ToVkQueueFlags(VulkanQueueFlags flags)
{
    VkQueueFlags vkflags = 0u;

    if (flags & VulkanQueueFlagBits::kGraphics)
    {
        vkflags |= VK_QUEUE_GRAPHICS_BIT;
    }
    if (flags & VulkanQueueFlagBits::kCompute)
    {
        vkflags |= VK_QUEUE_COMPUTE_BIT;
    }
    if (flags & VulkanQueueFlagBits::kTransfer)
    {
        vkflags |= VK_QUEUE_TRANSFER_BIT;
    }

    return vkflags;
}

VulkanQueueFlags ToQueueFlags(VkQueueFlags vkflags)
{
    VulkanQueueFlags flags = 0u;

    if (vkflags & VK_QUEUE_GRAPHICS_BIT)
    {
        flags |= VulkanQueueFlagBits::kGraphics;
    }
    if (vkflags & VK_QUEUE_COMPUTE_BIT)
    {
        flags |= VulkanQueueFlagBits::kCompute;
    }
    if (vkflags & VK_QUEUE_TRANSFER_BIT)
    {
        flags |= VulkanQueueFlagBits::kTransfer;
    }

    return flags;
}

} // namespace jipu