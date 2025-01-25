#pragma once

#include "export.h"

#include "buffer.h"
#include "jipu/common/cast.h"
#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_resource.h"

#include <memory>

namespace jipu
{

class VulkanDevice;
class CommandBuffer;
class VULKAN_EXPORT VulkanBuffer : public Buffer
{
public:
    VulkanBuffer() = delete;
    VulkanBuffer(VulkanDevice* device, const BufferDescriptor& descriptor) noexcept(false);
    ~VulkanBuffer() override;

    void* map() override;
    void unmap() override;

    BufferUsageFlags getUsage() const override;
    uint64_t getSize() const override;

public:
    void cmdPipelineBarrier(VkCommandBuffer commandBuffer,
                            VkPipelineStageFlags srcStage,
                            VkPipelineStageFlags dstStage,
                            VkBufferMemoryBarrier bufferBarrier);

    VulkanDevice* getDevice() const;
    VkBuffer getVkBuffer() const;
    VulkanMemory getVulkanMemory() const;
    VulkanBufferResource getVulkanBufferResource() const;

private:
    VulkanBufferResource m_resource;
    void* m_mappedPtr = nullptr;

private:
    VulkanDevice* m_device = nullptr;
    BufferDescriptor m_descriptor{};
};

DOWN_CAST(VulkanBuffer, Buffer);

// Convert Helper
VkAccessFlags ToVkAccessFlags(BufferUsageFlags flags);
VkBufferUsageFlags ToVkBufferUsageFlags(BufferUsageFlags usage);
VkPipelineStageFlags ToVkPipelineStageFlags(BufferUsageFlags usage);

// TODO: remove or remain.
// BufferUsageFlags ToBufferUsageFlags(VkAccessFlags vkflags);
// BufferUsageFlags ToBufferUsageFlags(VkBufferUsageFlags usages);
} // namespace jipu
