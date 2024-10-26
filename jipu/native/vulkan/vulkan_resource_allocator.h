#pragma once

#include "vulkan_export.h"
#include "vulkan_resource.h"

namespace jipu
{

struct VulkanResourceAllocatorDescriptor
{
};

class VulkanDevice;
class VULKAN_EXPORT VulkanResourceAllocator final
{
public:
    VulkanResourceAllocator() = delete;
    VulkanResourceAllocator(VulkanDevice& device, const VulkanResourceAllocatorDescriptor& descriptor);
    ~VulkanResourceAllocator();

    VulkanBufferResource createBuffer(const VkBufferCreateInfo& createInfo);
    void destroyBuffer(const VulkanBufferResource& bufferResource);

    VulkanTextureResource createTexture(const VkImageCreateInfo& createInfo);
    void destroyTexture(VulkanTextureResource textureResource);

    void* map(VulkanAllocation allocation);
    void unmap(VulkanAllocation allocation);

private:
    VulkanDevice& m_device;
#if defined(USE_VMA)
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VmaVulkanFunctions m_vmaFunctions{};
#endif
};

//

} // namespace jipu