#include "vulkan_submitter.h"

#include "vulkan_device.h"

#include <spdlog/spdlog.h>

namespace jipu
{

VulkanSubmitter::VulkanSubmitter(VulkanDevice* device)
    : m_device(device)
{
    const auto& activatedQueueFamilies = m_device->getActivatedQueueFamilies();
    if (activatedQueueFamilies.size() <= 0)
    {
        throw std::runtime_error("There is no activated queue familys.");
    }

    // collect all queues.
    QueueFamily queueFamilyCandidate{};
    for (auto activatedQueueFamilyIndex = 0; activatedQueueFamilyIndex < activatedQueueFamilies.size(); ++activatedQueueFamilyIndex)
    {
        auto& activatedQueueFamily = activatedQueueFamilies[activatedQueueFamilyIndex];
        auto queueFlags = activatedQueueFamily.queueFlags;

        // currently, only use one queue family.
        if ((queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            (queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            auto queueFamily = QueueFamily{ .graphicsQueues = {}, .transferQueue = VK_NULL_HANDLE };

            if (activatedQueueFamily.queueCount == 1)
            {
                VkQueue queue{ VK_NULL_HANDLE };
                m_device->vkAPI.GetDeviceQueue(m_device->getVkDevice(), activatedQueueFamilyIndex, 0, &queue);

                queueFamily.graphicsQueues.push_back(queue);
                queueFamily.transferQueue = queue;
            }
            else
            {
                // collect graphics and compute queue. (include transfer queue)
                for (auto queueIndex = 0; queueIndex < activatedQueueFamily.queueCount - 1; ++queueIndex)
                {
                    VkQueue queue{ VK_NULL_HANDLE };
                    m_device->vkAPI.GetDeviceQueue(m_device->getVkDevice(), activatedQueueFamilyIndex, queueIndex, &queue);

                    queueFamily.graphicsQueues.push_back(queue);
                }

                // set only one transfer queue.
                {
                    VkQueue queue{ VK_NULL_HANDLE };
                    m_device->vkAPI.GetDeviceQueue(m_device->getVkDevice(), activatedQueueFamilyIndex, activatedQueueFamily.queueCount - 1, &queue);

                    queueFamily.transferQueue = queue;
                }
            }

            if (queueFamily.graphicsQueues.size() > queueFamilyCandidate.graphicsQueues.size())
            {
                queueFamilyCandidate = queueFamily;
                break;
            }
        }
    }

    if (queueFamilyCandidate.graphicsQueues.empty())
    {
        throw std::runtime_error("There is no graphics queue.");
    }

    m_queueFamily = queueFamilyCandidate;
}

VulkanSubmitter::~VulkanSubmitter()
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    // wait idle state before destroy semaphore.
    for (auto queue : m_queueFamily.graphicsQueues)
        vkAPI.QueueWaitIdle(queue);
    vkAPI.QueueWaitIdle(m_queueFamily.transferQueue);

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

    auto queue = getVkQueue(SubmitType::kGraphics);
    VkResult result = vkAPI.QueueSubmit(queue, static_cast<uint32_t>(submitInfos.size()), submitInfos.data(), fence);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to submit command buffer {}", static_cast<uint32_t>(result)));
    }

    auto submitTask = [this, fence = fence, submits = submits]() -> void {
        const VulkanAPI& vkAPI = m_device->vkAPI;
        VkResult result = vkAPI.WaitForFences(m_device->getVkDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("failed to wait for fences {}", static_cast<uint32_t>(result)));
        }

        m_device->getInflightObjects()->clear(fence);
    };

    return m_threadPool.enqueue(submitTask);
}

void VulkanSubmitter::submit(VkFence fence, const std::vector<VulkanSubmit::Info>& submits)
{
    submitAsync(fence, submits).get();
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

    auto queue = getVkQueue(SubmitType::kPresent);
    vkAPI.QueuePresentKHR(queue, &info);
}

VkQueue VulkanSubmitter::getVkQueue(SubmitType type) const
{
    switch (type)
    {
    case SubmitType::kCompute:
    case SubmitType::kGraphics:
    case SubmitType::kPresent:
        return m_queueFamily.graphicsQueues[0]; // TODO: scheduling
        break;
    case SubmitType::kTransfer:
        return m_queueFamily.transferQueue;
    case SubmitType::kNone:
    default:
        throw std::runtime_error("There is no queue family properties.");
        break;
    }

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