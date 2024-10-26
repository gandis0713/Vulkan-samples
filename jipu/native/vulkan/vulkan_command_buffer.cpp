#include "vulkan_command_buffer.h"
#include "vulkan_command_encoder.h"
#include "vulkan_device.h"

#include <stdexcept>

namespace jipu
{

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandEncoder* commandEncoder, const CommandBufferDescriptor& descriptor)
    : m_commandEncoder(commandEncoder)
    , m_commandEncodingResult(m_commandEncoder->result())
{
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

const CommandEncodingResult& VulkanCommandBuffer::getCommandEncodingResult() const
{
    return m_commandEncodingResult;
}

VulkanCommandRecordResult VulkanCommandBuffer::recordToVkCommandBuffer()
{
    releaseVkCommandBuffer();
    createVkCommandBuffer();

    auto commandRecorder = createCommandRecorder();

    return commandRecorder->record();
}

VkCommandBuffer VulkanCommandBuffer::getVkCommandBuffer()
{
    return m_commandBuffer;
}

std::unique_ptr<VulkanCommandRecorder> VulkanCommandBuffer::createCommandRecorder()
{
    return std::make_unique<VulkanCommandRecorder>(this);
}

void VulkanCommandBuffer::createVkCommandBuffer()
{
    releaseVkCommandBuffer();

    m_commandBuffer = getDevice()->getCommandPool()->create();
}

void VulkanCommandBuffer::releaseVkCommandBuffer()
{
    if (m_commandBuffer)
    {
        getDevice()->getCommandPool()->release(m_commandBuffer);
        m_commandBuffer = VK_NULL_HANDLE;
    }
}

} // namespace jipu