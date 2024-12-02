#pragma once

#include "export.h"

#include "command_buffer.h"
#include "jipu/common/cast.h"

#include "vulkan_api.h"
#include "vulkan_command_encoder.h"
#include "vulkan_command_recorder.h"
#include "vulkan_command_resource_synchronizer.h"
#include "vulkan_export.h"

#include <vector>

namespace jipu
{

class VulkanDevice;
class VulkanCommandEncoder;
class VULKAN_EXPORT VulkanCommandBuffer : public CommandBuffer
{
public:
    VulkanCommandBuffer() = delete;
    VulkanCommandBuffer(VulkanCommandEncoder* commandEncoder, const CommandBufferDescriptor& descriptor);
    ~VulkanCommandBuffer() override;

public:
    VulkanDevice* getDevice() const;
    VulkanCommandEncoder* getCommandEncoder() const;
    const CommandEncodingResult& getCommandEncodingResult() const;

public:
    VulkanCommandRecordResult recordToVkCommandBuffer();

public:
    VkCommandBuffer getVkCommandBuffer();

private:
    std::unique_ptr<VulkanCommandRecorder> createCommandRecorder();

private:
    void createVkCommandBuffer();
    void releaseVkCommandBuffer();

private:
    VulkanCommandEncoder* m_commandEncoder = nullptr;
    CommandEncodingResult m_commandEncodingResult{};

private:
    // store VkCommandBuffer to reuse it as secondary command buffer if need.
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
};

DOWN_CAST(VulkanCommandBuffer, CommandBuffer);

} // namespace jipu