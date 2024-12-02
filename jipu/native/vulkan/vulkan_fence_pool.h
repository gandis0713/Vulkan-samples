#pragma once

#include "vulkan_api.h"

#include <unordered_map>

namespace jipu
{

class VulkanDevice;
class VulkanFencePool final
{
public:
    VulkanFencePool() = delete;
    explicit VulkanFencePool(VulkanDevice* device);
    ~VulkanFencePool();

public:
    VkFence create();
    void release(VkFence fence);

private:
    VulkanDevice* m_device = nullptr;

private:
    std::unordered_map<VkFence, bool> m_fences{};
};

} // namespace jipu