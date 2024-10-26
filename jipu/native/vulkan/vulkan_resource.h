#pragma once

#include "vulkan_api.h" // should be included before 'vk_mem_alloc.h'

// #define USE_VMA 1

#if defined(USE_VMA)
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

using VulkanAllocation = VmaAllocation;
#else
using VulkanAllocation = VkDeviceMemory;
#endif

namespace jipu
{

struct VulkanBufferResource
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VulkanAllocation allocation = VK_NULL_HANDLE;
};

struct VulkanTextureResource
{
    VkImage image = VK_NULL_HANDLE;
    VulkanAllocation allocation = VK_NULL_HANDLE;
};

} // namespace jipu