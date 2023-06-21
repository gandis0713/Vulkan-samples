#include "vulkan_queue.h"
#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_synchronization.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace vkt
{

VulkanQueue::VulkanQueue(VulkanDevice* device, const QueueDescriptor& descriptor) noexcept(false)
    : Queue(device, descriptor)
{
    VulkanPhysicalDevice* physicalDevice = downcast(m_device->getPhysicalDevice());

    const VulkanPhysicalDeviceInfo& deviceInfo = physicalDevice->getInfo();

    const uint32_t queueFamilyPropertiesSize = deviceInfo.queueFamilyProperties.size();
    if (queueFamilyPropertiesSize <= 0)
    {
        throw std::runtime_error("There is no queue family properties.");
    }

    for (uint32_t index = 0; index < queueFamilyPropertiesSize; ++index)
    {
        const auto& properties = deviceInfo.queueFamilyProperties[index];
        if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_index = index;
            m_properties = properties;
            break;
        }
    }

    device->vkAPI.GetDeviceQueue(device->getVkDevice(), m_index, 0, &m_queue);

    // create semaphore
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    if (device->vkAPI.CreateSemaphore(device->getVkDevice(), &semaphoreCreateInfo, nullptr, &m_renderingFinishSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create rendering queue semephore.");
    }
}

VulkanQueue::~VulkanQueue()
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    // wait idle state before destroy semaphore.
    vkAPI.QueueWaitIdle(m_queue);

    // Doesn't need to destroy VkQueue.

    vkAPI.DestroySemaphore(vulkanDevice->getVkDevice(), m_renderingFinishSemaphore, nullptr);
}

void VulkanQueue::submit(CommandBuffer* commandBuffer)
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    VulkanSynchronization& sync = vulkanDevice->getSynchronization();

    SemaphoreDescriptor waitDescriptor{ .type = SemephoreType::kHostToQueue };
    std::vector<VkSemaphore> waitSemaphores = sync.takeoutSemaphore(waitDescriptor);
    SemaphoreDescriptor signalDescriptor{ .type = SemephoreType::kQueueToHost };
    sync.injectSemaphore(signalDescriptor, m_renderingFinishSemaphore);

    // submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitPipelineStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitPipelineStages;

    submitInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuf = downcast(commandBuffer)->getVkCommandBuffer();
    submitInfo.pCommandBuffers = &commandBuf;

    VkSemaphore signalSemaphores[] = { m_renderingFinishSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkAPI.QueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        spdlog::error("failed to submit draw command buffer!");
    }

    // TODO: check really need below?
    vulkanDevice->vkAPI.QueueWaitIdle(m_queue);
}

VkQueue VulkanQueue::getVkQueue() const
{
    return m_queue;
}

// Convert Helper
VkQueueFlags ToVkQueueFlags(QueueFlags flags)
{
    VkQueueFlags vkflags = 0x00000000; // 0x00000000

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
    QueueFlags flags = QueueFlagBits::kUndefined; // 0x00000000

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

} // namespace vkt
