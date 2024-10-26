#define VMA_IMPLEMENTATION
#include "vulkan_resource_allocator.h"

#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "vulkan_physical_device.h"
#include "vulkan_texture.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace jipu
{

namespace
{

#if defined(DISABLE)
void* VKAPI_PTR allocateCB(void* data, size_t size, size_t alignment, VkSystemAllocationScope scope)
{
    void* p = aligned_alloc(alignment, size);

    spdlog::trace("allocate of size {}, alignment {}, scope {}, address {}", size, alignment, static_cast<uint32_t>(scope), p);

    return p;
}

void VKAPI_PTR freeCB(void* data, void* memory)
{
    spdlog::trace("free address {}", memory);
    free(memory);
}

void* VKAPI_PTR reallocateCB(void* data, void* original, size_t size, size_t alignment, VkSystemAllocationScope scope)
{
    free(original);
    void* p = aligned_alloc(alignment, size);

    spdlog::trace("reallocate of size {}, alignment {}, scope {}, original address {}, reallocate address {}", size, alignment, static_cast<uint32_t>(scope), original, p);

    return p;
}

void VKAPI_PTR internalAllocationNotificationCB(void* data,
                                                size_t size,
                                                VkInternalAllocationType type,
                                                VkSystemAllocationScope scope)
{
    spdlog::trace("internal allocation of size {}, type {}, scope", size, static_cast<uint32_t>(type), static_cast<uint32_t>(scope));
}

void VKAPI_PTR internalFreeNotificationCB(void* data,
                                          size_t size,
                                          VkInternalAllocationType type,
                                          VkSystemAllocationScope scope)
{
    spdlog::trace("internal free of size {}, type {}, scope", size, static_cast<uint32_t>(type), static_cast<uint32_t>(scope));
}
#endif

#if defined(USE_VMA)
VulkanBufferResource createBufferResource(VmaAllocator allocator, const VkBufferCreateInfo& createInfo)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // TODO : set usage from createInfo.

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation;
    VkResult result = vmaCreateBuffer(allocator, &createInfo, &allocInfo, &buffer, &allocation, nullptr);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create buffer. error: {}", static_cast<int32_t>(result)));
    }

    return { .buffer = buffer, .allocation = allocation };
}
void destroyBufferResource(VmaAllocator allocator, const VulkanBufferResource& bufferMemory)
{
    vmaDestroyBuffer(allocator, bufferMemory.buffer, bufferMemory.allocation);
}

VulkanTextureResource createTextureResource(VmaAllocator allocator, const VkImageCreateInfo& createInfo)
{
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; // TODO: set usage from createInfo.

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation;
    VkResult result = vmaCreateImage(allocator, &createInfo, &allocInfo, &image, &allocation, nullptr);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create image. error: {}", static_cast<int32_t>(result)));
    }

    return { .image = image, .allocation = allocation };
}
void destroyTextureResource(VmaAllocator allocator, const VulkanTextureResource& textureResource)
{
    vmaDestroyImage(allocator, textureResource.image, textureResource.allocation);
}
void* mapResource(VmaAllocator allocator, VmaAllocation allocation)
{
    void* data = nullptr;
    VkResult result = vmaMapMemory(allocator, allocation, &data);
    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to map to pointer. error: {}", static_cast<int32_t>(result));
    }
    return data;
}
void unmapResource(VmaAllocator allocator, VmaAllocation allocation)
{
    vmaUnmapMemory(allocator, allocation);
}
#else
VulkanBufferResource createBufferResource(VulkanDevice& device, const VkBufferCreateInfo& createInfo)
{
    const VulkanAPI& vkAPI = device.vkAPI;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkResult result = vkAPI.CreateBuffer(device.getVkDevice(), &createInfo, nullptr, &buffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create buffer. error: {}", static_cast<int32_t>(result)));
    }

    VkMemoryRequirements memoryRequirements{};
    vkAPI.GetBufferMemoryRequirements(device.getVkDevice(), buffer, &memoryRequirements);

    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; // TODO: set memory property flags by create information.

    int memoryTypeIndex = downcast(device.getPhysicalDevice()).findMemoryTypeIndex(memoryPropertyFlags);
    if (memoryTypeIndex == -1)
    {
        throw std::runtime_error("Failed to find memory type index");
    }

    VkMemoryAllocateInfo memoryAllocateInfo{ .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                             .allocationSize = memoryRequirements.size,
                                             .memoryTypeIndex = static_cast<uint32_t>(memoryTypeIndex) };

    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    result = vkAPI.AllocateMemory(device.getVkDevice(), &memoryAllocateInfo, nullptr, &deviceMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate memory");
    }

    result = vkAPI.BindBufferMemory(device.getVkDevice(), buffer, deviceMemory, 0);
    if (result != VK_SUCCESS)
    {
        // TODO: delete VkBuffer resource automatically.
        device.vkAPI.DestroyBuffer(device.getVkDevice(), buffer, nullptr);

        throw std::runtime_error("Failed to bind memory");
    }

    return { .buffer = buffer, .allocation = deviceMemory };
}

void destroyBufferResource(VulkanDevice& device, const VulkanBufferResource& bufferResource)
{
    device.vkAPI.FreeMemory(device.getVkDevice(), bufferResource.allocation, nullptr);
    device.vkAPI.DestroyBuffer(device.getVkDevice(), bufferResource.buffer, nullptr);
}

VulkanTextureResource createTextureResource(VulkanDevice& device, const VkImageCreateInfo& createInfo)
{
    const VulkanAPI& vkAPI = device.vkAPI;
    VkImage image = VK_NULL_HANDLE;
    VkResult result = vkAPI.CreateImage(device.getVkDevice(), &createInfo, nullptr, &image);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create image. error: {}", static_cast<int32_t>(result)));
    }

    VkMemoryRequirements memoryRequirements{};
    vkAPI.GetImageMemoryRequirements(device.getVkDevice(), image, &memoryRequirements);

    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // TODO: set memory property flags by create information.

    int memoryTypeIndex = downcast(device.getPhysicalDevice()).findMemoryTypeIndex(memoryPropertyFlags);
    if (memoryTypeIndex == -1)
    {
        throw std::runtime_error("Failed to find memory type index");
    }

    VkMemoryAllocateInfo memoryAllocateInfo{ .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                             .allocationSize = memoryRequirements.size,
                                             .memoryTypeIndex = static_cast<uint32_t>(memoryTypeIndex) };

    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    result = vkAPI.AllocateMemory(device.getVkDevice(), &memoryAllocateInfo, nullptr, &deviceMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate memory");
    }

    result = vkAPI.BindImageMemory(device.getVkDevice(), image, deviceMemory, 0);
    if (result != VK_SUCCESS)
    {
        // TODO: delete VkImage resource automatically.
        device.vkAPI.DestroyImage(device.getVkDevice(), image, nullptr);

        throw std::runtime_error(fmt::format("Failed to bind memory. {}", static_cast<int32_t>(result)));
    }

    return { .image = image, .allocation = deviceMemory };
}
void destroyTextureResource(VulkanDevice& device, const VulkanTextureResource& textureResource)
{
    device.vkAPI.FreeMemory(device.getVkDevice(), textureResource.allocation, nullptr);
    device.vkAPI.DestroyImage(device.getVkDevice(), textureResource.image, nullptr);
}
void* mapResource(VulkanDevice& device, VulkanAllocation allocation)
{
    void* data = nullptr;
    VkResult result = device.vkAPI.MapMemory(device.getVkDevice(), allocation, 0, VK_WHOLE_SIZE, 0, &data);
    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to map to pointer. error: {}", static_cast<int32_t>(result));
    }
    return data;
}
void unmapResource(VulkanDevice& device, VkDeviceMemory memory)
{
    device.vkAPI.UnmapMemory(device.getVkDevice(), memory);
}
#endif

} // namespace

VulkanResourceAllocator::VulkanResourceAllocator(VulkanDevice& device, const VulkanResourceAllocatorDescriptor& descriptor)
    : m_device(device)
{
#if defined(USE_VMA)
    auto& vulkanPhysicalDevice = downcast(m_device.getPhysicalDevice());
    auto& vulkanInstance = downcast(vulkanPhysicalDevice.getInstance());

    auto physicalDevice = vulkanPhysicalDevice.getVkPhysicalDevice();
    auto instance = vulkanInstance.getVkInstance();

#if defined(VMA_DYNAMIC_VULKAN_FUNCTIONS)
    m_vmaFunctions.vkGetInstanceProcAddr = vulkanInstance.vkAPI.GetInstanceProcAddr;
    m_vmaFunctions.vkGetDeviceProcAddr = m_device.vkAPI.GetDeviceProcAddr;
#else
    // TODO: set functions
#endif

    VmaAllocatorCreateInfo createInfo{};
    createInfo.instance = instance;
    createInfo.physicalDevice = physicalDevice;
    createInfo.device = m_device.getVkDevice();
    createInfo.vulkanApiVersion = vulkanInstance.getInstanceInfo().apiVersion;
    createInfo.pVulkanFunctions = &m_vmaFunctions;

#if defined(DISABLE)
    VkAllocationCallbacks allocCallbacks = {
        nullptr,
        allocateCB,
        reallocateCB,
        freeCB,
        internalAllocationNotificationCB,
        internalFreeNotificationCB
    };
    createInfo.pAllocationCallbacks = &allocCallbacks;
}
#endif

VkResult result = vmaCreateAllocator(&createInfo, &m_allocator);
if (result != VK_SUCCESS)
{
    throw std::runtime_error("Failed to create vma allocator");
}
#endif
}

VulkanResourceAllocator::~VulkanResourceAllocator()
{
#if defined(USE_VMA)
    vmaDestroyAllocator(m_allocator);
#endif
}

VulkanBufferResource VulkanResourceAllocator::createBuffer(const VkBufferCreateInfo& createInfo)
{
#if defined(USE_VMA)
    return createBufferResource(m_allocator, createInfo);
#else
    return createBufferResource(m_device, createInfo);
#endif
}

void VulkanResourceAllocator::destroyBuffer(const VulkanBufferResource& bufferResource)
{
#if defined(USE_VMA)
    destroyBufferResource(m_allocator, bufferResource);
#else
    destroyBufferResource(m_device, bufferResource);
#endif
}

VulkanTextureResource VulkanResourceAllocator::createTexture(const VkImageCreateInfo& createInfo)
{
#if defined(USE_VMA)
    return createTextureResource(m_allocator, createInfo);
#else
    return createTextureResource(m_device, createInfo);
#endif
}

void VulkanResourceAllocator::destroyTexture(VulkanTextureResource textureResource)
{
#if defined(USE_VMA)
    destroyTextureResource(m_allocator, textureResource);
#else
    destroyTextureResource(m_device, textureResource);
#endif
}

void* VulkanResourceAllocator::map(VulkanAllocation allocation)
{
#if defined(USE_VMA)
    return mapResource(m_allocator, allocation);
#else
    return mapResource(m_device, allocation);
#endif
}

void VulkanResourceAllocator::unmap(VulkanAllocation allocation)
{
#if defined(USE_VMA)
    unmapResource(m_allocator, allocation);
#else
    unmapResource(m_device, allocation);
#endif
}

} // namespace jipu