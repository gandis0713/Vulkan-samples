#include "vulkan_command_buffer.h"
#include "vulkan_command_encoder.h"
#include "vulkan_device.h"

#include <stdexcept>

namespace jipu
{

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandEncoder* commandEncoder, const CommandBufferDescriptor& descriptor)
    : m_commandEncoder(commandEncoder)
{
    createVkCommandBuffer();

    recordToVkCommandBuffer();
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    releaseVkCommandBuffer();
}

VulkanDevice* VulkanCommandBuffer::getDevice() const
{
    return m_commandEncoder->getDevice();
}

VulkanCommandEncoder* VulkanCommandBuffer::getCommandEncoder() const
{
    return m_commandEncoder;
}
const std::vector<std::unique_ptr<Command>>& VulkanCommandBuffer::getCommands()
{
    return m_commandRecordResult.commands;
}

const std::vector<OperationResourceInfo>& VulkanCommandBuffer::getCommandResourceInfos()
{
    return m_commandRecordResult.resourceSyncResult.notSyncedOperationResourceInfos;
}

void VulkanCommandBuffer::recordToVkCommandBuffer()
{
    auto encodingReslut = m_commandEncoder->finish();
    auto commandRecorder = std::make_unique<VulkanCommandRecorder>(this,
                                                                   VulkanCommandRecorderDescriptor{ .commandEncodingResult = std::move(encodingReslut) });

    m_commandRecordResult = commandRecorder->record();
}

VkCommandBuffer VulkanCommandBuffer::getVkCommandBuffer()
{
    return m_commandBuffer;
}

void VulkanCommandBuffer::createVkCommandBuffer()
{
    if (!m_commandBuffer)
    {
        VulkanCommandBufferDescriptor descriptor{ .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY };
        m_commandBuffer = getDevice()->getCommandPool()->create(descriptor);
    }
}

void VulkanCommandBuffer::releaseVkCommandBuffer()
{
    if (m_commandBuffer)
    {
        getDevice()->getDeleter()->safeDestroy(m_commandBuffer);
        m_commandBuffer = VK_NULL_HANDLE;
    }
}

} // namespace jipu