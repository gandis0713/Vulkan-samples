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

    for (auto index = 0; index < queueFamilies.size(); ++index)
    {
        VkQueue queue{ VK_NULL_HANDLE };
        m_device->vkAPI.GetDeviceQueue(m_device->getVkDevice(), index, 0, &queue);
        m_queues.push_back({ queue, ToQueueFlags(queueFamilies[index].queueFlags) });
    }

    // create fence.
    m_fence = m_device->getFencePool()->create();
}

VulkanSubmitter::~VulkanSubmitter()
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    // wait idle state before destroy semaphore.
    for (auto& [queue, _] : m_queues)
    {
        vkAPI.QueueWaitIdle(queue);
    }

    vulkanDevice->getFencePool()->release(m_fence);

    // Doesn't need to destroy VkQueue.
}

void VulkanSubmitter::submit(const std::vector<VulkanSubmit::Info>& submits)
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

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

    auto queue = getVkQueue(0u); // TODO: get by queue flags
    VkResult result = vkAPI.QueueSubmit(queue, static_cast<uint32_t>(submitInfos.size()), submitInfos.data(), m_fence);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to submit command buffer {}", static_cast<uint32_t>(result)));
    }

    result = vkAPI.WaitForFences(vulkanDevice->getVkDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to wait for fences {}", static_cast<uint32_t>(result)));
    }

    result = vkAPI.ResetFences(vulkanDevice->getVkDevice(), 1, &m_fence);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to reset for fences {}", static_cast<uint32_t>(result)));
    }
}

void VulkanSubmitter::present(std::vector<VulkanSubmit::Info> submitInfos, VulkanPresentInfo presentInfo)
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    auto queue = getVkQueue(0u); // TODO: get by queue flags

    // prepare submit and present infos
    {
        // add acquire image semaphore to submit infos.
        for (auto& submitInfo : submitInfos)
        {
            if (submitInfo.type == SubmitType::kPresent)
            {
                submitInfo.waitSemaphores.insert(submitInfo.waitSemaphores.end(),
                                                 presentInfo.signalSemaphore.begin(),
                                                 presentInfo.signalSemaphore.end());
                submitInfo.waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            }
        }

        // add render semaphore to present infos.
        for (const auto& submitInfo : submitInfos)
        {
            if (submitInfo.type == SubmitType::kPresent)
            {
                presentInfo.waitSemaphores.insert(presentInfo.waitSemaphores.end(), submitInfo.signalSemaphores.begin(), submitInfo.signalSemaphores.end());
            }
        }
    }

    // submit
    submit(submitInfos);

    // present
    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = static_cast<uint32_t>(presentInfo.waitSemaphores.size());
    info.pWaitSemaphores = presentInfo.waitSemaphores.data();
    info.swapchainCount = static_cast<uint32_t>(presentInfo.swapchains.size());
    info.pSwapchains = presentInfo.swapchains.data();
    info.pImageIndices = presentInfo.imageIndices.data();
    info.pResults = nullptr; // Optional

    vkAPI.QueuePresentKHR(queue, &info);
}

VkQueue VulkanSubmitter::getVkQueue(uint32_t index) const
{
    assert(m_queues.size() > index);

    return m_queues[index].first;
}

VkQueue VulkanSubmitter::getVkQueue(VulkanQueueFlags flags) const
{
    for (const auto& [queue, queueFlags] : m_queues)
    {
        if (queueFlags & flags)
        {
            return queue;
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