#include "vulkan_queue.h"

#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include "vulkan_submit_context.h"
#include "vulkan_swapchain.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanQueue::VulkanQueue(VulkanDevice& device, const QueueDescriptor& descriptor) noexcept(false)
    : m_device(device)
{
    VulkanPhysicalDevice& physicalDevice = device.getPhysicalDevice();

    const VulkanPhysicalDeviceInfo& deviceInfo = physicalDevice.getVulkanPhysicalDeviceInfo();

    const uint64_t queueFamilyPropertiesSize = deviceInfo.queueFamilyProperties.size();
    if (queueFamilyPropertiesSize <= 0)
    {
        throw std::runtime_error("There is no queue family properties.");
    }

    for (uint64_t index = 0; index < queueFamilyPropertiesSize; ++index)
    {
        const auto& properties = deviceInfo.queueFamilyProperties[index];
        if (properties.queueFlags & ToVkQueueFlags(descriptor.flags))
        {
            m_index = static_cast<uint32_t>(index);
            m_properties = properties;
            break;
        }
    }

    device.vkAPI.GetDeviceQueue(device.getVkDevice(), m_index, 0, &m_queue);

    // create semaphore
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    if (device.vkAPI.CreateSemaphore(m_device.getVkDevice(), &semaphoreCreateInfo, nullptr, &m_semaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create semaphore in queue.");
    }

    // create fence.
    m_fence = m_device.getFencePool()->create();
}

VulkanQueue::~VulkanQueue()
{
    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    // wait idle state before destroy semaphore.
    vkAPI.QueueWaitIdle(m_queue);

    m_device.getFencePool()->release(m_fence);
    vkAPI.DestroySemaphore(vulkanDevice.getVkDevice(), m_semaphore, nullptr);

    // Doesn't need to destroy VkQueue.
}

void VulkanQueue::submit(std::vector<CommandBuffer*> commandBuffers)
{
    // record VulkanCommandBuffer to VkCommandBuffer.
    std::vector<VulkanCommandRecordResult> commandRecordResults = recordCommands(commandBuffers);

    // generate SubmitInfo.
    VulkanSubmitContext submitContext = VulkanSubmitContext::create(&m_device, commandRecordResults);
    auto submitInfos = submitContext.getSubmitInfos();

    auto isPresentSubmit = [&](const auto& submitInfos) -> bool {
        for (const auto& submitInfo : submitInfos)
        {
            if (submitInfo.type == SubmitType::kPresent)
                return true;
        }
        return false;
    }(submitInfos);

    if (isPresentSubmit)
    {
        m_presentSubmitInfos = submitInfos;
    }
    else
    {
        submit(submitInfos);
    }
}

void VulkanQueue::submit(std::vector<CommandBuffer*> commandBuffers, Swapchain& swapchain)
{
    submit(commandBuffers);
    swapchain.present();
}

void VulkanQueue::present(VulkanPresentInfo presentInfo)
{
    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    // prepare submit and present infos
    {
        // add acquire image semaphore to submit infos.
        for (auto& submitInfo : m_presentSubmitInfos)
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
        for (const auto& submitInfo : m_presentSubmitInfos)
        {
            if (submitInfo.type == SubmitType::kPresent)
            {
                presentInfo.waitSemaphores.insert(presentInfo.waitSemaphores.end(), submitInfo.signalSemaphores.begin(), submitInfo.signalSemaphores.end());
            }
        }
    }

    // submit
    {
        submit(m_presentSubmitInfos);
        m_presentSubmitInfos = {};
    }

    // present
    {
        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = static_cast<uint32_t>(presentInfo.waitSemaphores.size());
        info.pWaitSemaphores = presentInfo.waitSemaphores.data();
        info.swapchainCount = static_cast<uint32_t>(presentInfo.swapchains.size());
        info.pSwapchains = presentInfo.swapchains.data();
        info.pImageIndices = presentInfo.imageIndices.data();
        info.pResults = nullptr; // Optional

        vkAPI.QueuePresentKHR(m_queue, &info);
    }
}

VkQueue VulkanQueue::getVkQueue() const
{
    return m_queue;
}

std::vector<VkSemaphore> VulkanQueue::getSemaphores() const
{
    return { m_semaphore };
}

std::vector<VulkanCommandRecordResult> VulkanQueue::recordCommands(std::vector<CommandBuffer*> commandBuffers)
{
    std::vector<VulkanCommandRecordResult> commandRecordResult{};

    for (auto& commandBuffer : commandBuffers)
    {
        auto vulkanCommandBuffer = downcast(commandBuffer);
        commandRecordResult.push_back(vulkanCommandBuffer->recordToVkCommandBuffer());
    }

    return commandRecordResult;
}

void VulkanQueue::submit(const std::vector<VulkanSubmit::Info>& submits)
{
    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

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

    VkResult result = vkAPI.QueueSubmit(m_queue, static_cast<uint32_t>(submitInfos.size()), submitInfos.data(), m_fence);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to submit command buffer {}", static_cast<uint32_t>(result)));
    }

    result = vkAPI.WaitForFences(vulkanDevice.getVkDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to wait for fences {}", static_cast<uint32_t>(result)));
    }

    result = vkAPI.ResetFences(vulkanDevice.getVkDevice(), 1, &m_fence);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to reset for fences {}", static_cast<uint32_t>(result)));
    }
}

// Convert Helper
VkQueueFlags ToVkQueueFlags(QueueFlags flags)
{
    VkQueueFlags vkflags = 0u;

    if (flags & QueueFlagBits::kGraphics)
    {
        vkflags |= VK_QUEUE_GRAPHICS_BIT;
    }
    if (flags & QueueFlagBits::kCompute)
    {
        vkflags |= VK_QUEUE_COMPUTE_BIT;
    }
    if (flags & QueueFlagBits::kTransfer)
    {
        vkflags |= VK_QUEUE_TRANSFER_BIT;
    }

    return vkflags;
}

QueueFlags ToQueueFlags(VkQueueFlags vkflags)
{
    QueueFlags flags = 0u;

    if (vkflags & VK_QUEUE_GRAPHICS_BIT)
    {
        flags |= QueueFlagBits::kGraphics;
    }
    if (vkflags & VK_QUEUE_COMPUTE_BIT)
    {
        flags |= QueueFlagBits::kCompute;
    }
    if (vkflags & VK_QUEUE_TRANSFER_BIT)
    {
        flags |= QueueFlagBits::kTransfer;
    }

    return flags;
}

} // namespace jipu
