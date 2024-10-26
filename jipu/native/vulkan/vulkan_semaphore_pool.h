#pragma once

#include "vulkan_api.h"

#include <unordered_map>

namespace jipu
{

class VulkanDevice;
class VulkanSemaphorePool final
{
public:
    VulkanSemaphorePool() = delete;
    explicit VulkanSemaphorePool(VulkanDevice* device);
    ~VulkanSemaphorePool();

public:
    VkSemaphore create();
    void release(VkSemaphore semaphore);

private:
    VulkanDevice* m_device = nullptr;

private:
    std::unordered_map<VkSemaphore, bool> m_semaphores{};
};

} // namespace jipu