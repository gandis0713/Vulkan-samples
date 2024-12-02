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
    VulkanResourceAllocator(VulkanDevice* device, const VulkanResourceAllocatorDescriptor& descriptor);
    ~VulkanResourceAllocator();

    VulkanBufferResource createBufferResource(const VkBufferCreateInfo& createInfo);
    void destroyBufferResource(const VulkanBufferResource& bufferResource);

    VulkanTextureResource createTextureResource(const VkImageCreateInfo& createInfo);
    void destroyTextureResource(VulkanTextureResource textureResource);

    void* map(VulkanMemory allocation);
    void unmap(VulkanMemory allocation);

private:
    VulkanDevice* m_device = nullptr;
#if defined(USE_VMA)
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VmaVulkanFunctions m_vmaFunctions{};
#endif
};

//

} // namespace jipu