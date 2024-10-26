#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include "vulkan_resource_allocator.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanBuffer::VulkanBuffer(VulkanDevice& device, const BufferDescriptor& descriptor) noexcept(false)
    : m_device(device)
    , m_descriptor(descriptor)
{
    if (descriptor.size == 0)
    {
        throw std::runtime_error("Buffer size must be greater than 0.");
    }

    if (descriptor.usage == BufferUsageFlagBits::kUndefined)
    {
        throw std::runtime_error("Buffer usage must not be undefined.");
    }

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = descriptor.size;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.usage = ToVkBufferUsageFlags(descriptor.usage);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto& vulkanResourceAllocator = device.getResourceAllocator();
    m_resource = vulkanResourceAllocator.createBuffer(bufferCreateInfo);
}

VulkanBuffer::~VulkanBuffer()
{
    unmap();

    auto& vulkanResourceAllocator = downcast(m_device).getResourceAllocator();
    vulkanResourceAllocator.destroyBuffer(m_resource);
}

void* VulkanBuffer::map()
{
    if (m_mappedPtr == nullptr)
    {
        auto& resourceAllocator = downcast(m_device).getResourceAllocator();
        m_mappedPtr = resourceAllocator.map(m_resource.allocation);
    }

    return m_mappedPtr;
}
void VulkanBuffer::unmap()
{
    if (m_mappedPtr)
    {
        auto& resourceAllocator = downcast(m_device).getResourceAllocator();
        resourceAllocator.unmap(m_resource.allocation);

        m_mappedPtr = nullptr;
    }
}

BufferUsageFlags VulkanBuffer::getUsage() const
{
    return m_descriptor.usage;
}

uint64_t VulkanBuffer::getSize() const
{
    return m_descriptor.size;
}

void VulkanBuffer::setTransition(VkCommandBuffer commandBuffer, VkPipelineStageFlags flags)
{
    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = ToVkAccessFlags(m_descriptor.usage);
    barrier.dstAccessMask = ToVkAccessFlags(m_descriptor.usage);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = m_resource.buffer;
    barrier.size = getSize();
    barrier.offset = 0;

    vkAPI.CmdPipelineBarrier(commandBuffer, m_stageFlags, flags, 0, 0, nullptr, 1, &barrier, 0, nullptr);

    m_stageFlags = flags;
}

VkBuffer VulkanBuffer::getVkBuffer() const
{
    return m_resource.buffer;
}

// Convert Helper
VkAccessFlags ToVkAccessFlags(BufferUsageFlags usage)
{
    VkAccessFlags vkFlags = 0x00000000; // 0x00000000

    if (usage & BufferUsageFlagBits::kMapRead)
    {
        vkFlags |= VK_ACCESS_HOST_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kMapWrite)
    {
        vkFlags |= VK_ACCESS_HOST_WRITE_BIT;
    }
    if (usage & BufferUsageFlagBits::kCopySrc)
    {
        vkFlags |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kCopyDst)
    {
        vkFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (usage & BufferUsageFlagBits::kIndex)
    {
        vkFlags |= VK_ACCESS_INDEX_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kVertex)
    {
        vkFlags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kUniform)
    {
        vkFlags |= VK_ACCESS_UNIFORM_READ_BIT;
    }

    return vkFlags;
}

VkPipelineStageFlags ToVkPipelineStageFlags(BufferUsageFlags usage)
{
    VkPipelineStageFlags flags = 0;

    if (usage & (BufferUsageFlagBits::kMapRead | BufferUsageFlagBits::kMapWrite))
    {
        flags |= VK_PIPELINE_STAGE_HOST_BIT;
    }
    if (usage & (BufferUsageFlagBits::kCopySrc | BufferUsageFlagBits::kCopyDst))
    {
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if (usage & (BufferUsageFlagBits::kIndex | BufferUsageFlagBits::kVertex))
    {
        flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    }
    if (usage & (BufferUsageFlagBits::kUniform | BufferUsageFlagBits::kStorage))
    {
        // TODO: set by shader stage.
        flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }

    return flags;
}

VkBufferUsageFlags ToVkBufferUsageFlags(BufferUsageFlags usages)
{
    VkBufferUsageFlags vkUsages = 0x00000000;

    if (usages & BufferUsageFlagBits::kCopySrc)
    {
        vkUsages |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (usages & BufferUsageFlagBits::kCopyDst)
    {
        vkUsages |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (usages & BufferUsageFlagBits::kIndex)
    {
        vkUsages |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (usages & BufferUsageFlagBits::kVertex)
    {
        vkUsages |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (usages & BufferUsageFlagBits::kUniform)
    {
        vkUsages |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (usages & BufferUsageFlagBits::kStorage)
    {
        vkUsages |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (usages & BufferUsageFlagBits::kQueryResolve)
    {
        vkUsages |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    // TODO: kMapRead, kMapWrite

    return vkUsages;
}

// BufferUsageFlags ToBufferUsageFlags(VkAccessFlags vkflags)
// {
//     BufferUsageFlags flags = BufferUsageFlagBits::kInvalid; // 0x00000000

//     if (vkflags & VK_ACCESS_HOST_READ_BIT)
//     {
//         flags |= BufferUsageFlagBits::kMapRead;
//     }
//     if (vkflags & VK_ACCESS_HOST_WRITE_BIT)
//     {
//         flags |= BufferUsageFlagBits::kMapWrite;
//     }
//     if (vkflags & VK_ACCESS_TRANSFER_READ_BIT)
//     {
//         flags |= BufferUsageFlagBits::kCopySrc;
//     }
//     if (vkflags & VK_ACCESS_TRANSFER_WRITE_BIT)
//     {
//         flags |= BufferUsageFlagBits::kCopyDst;
//     }
//     if (vkflags & VK_ACCESS_INDEX_READ_BIT)
//     {
//         flags = BufferUsageFlagBits::kIndex | BufferUsageFlagBits::kCopyDst;
//     }
//     if (vkflags & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)
//     {
//         flags = flags | BufferUsageFlagBits::kVertex;
//     }
//     if (vkflags & VK_ACCESS_UNIFORM_READ_BIT)
//     {
//         flags |= BufferUsageFlagBits::kUniform;
//     }

//     return flags;
// }

// BufferUsageFlags ToBufferUsageFlags(VkBufferUsageFlags vkUsages)
// {
//     BufferUsageFlags usages = BufferUsageFlagBits::kInvalid;

//     if (vkUsages & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
//     {
//         usages |= BufferUsageFlagBits::kCopySrc;
//     }
//     if (vkUsages & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
//     {
//         usages |= BufferUsageFlagBits::kCopyDst;
//     }
//     if (vkUsages & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
//     {
//         usages |= BufferUsageFlagBits::kIndex;
//     }
//     if (vkUsages & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
//     {
//         usages |= BufferUsageFlagBits::kVertex;
//     }
//     if (vkUsages & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
//     {
//         usages |= BufferUsageFlagBits::kUniform;
//     }

//     return usages;
// }
} // namespace jipu
