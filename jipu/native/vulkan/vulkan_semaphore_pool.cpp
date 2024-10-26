#include "vulkan_semaphore_pool.h"

#include "vulkan_device.h"

#include <spdlog/spdlog.h>

namespace jipu
{

VulkanSemaphorePool::VulkanSemaphorePool(VulkanDevice* device)
    : m_device(device)
{
}

VulkanSemaphorePool::~VulkanSemaphorePool()
{
    for (auto& semaphore : m_semaphores)
    {
        if (semaphore.second)
        {
            spdlog::warn("Semaphore is not released in this semaphore pool.");
        }

        m_device->vkAPI.DestroySemaphore(m_device->getVkDevice(), semaphore.first, nullptr);
    }
}

VkSemaphore VulkanSemaphorePool::create()
{
    for (auto& semaphore : m_semaphores)
    {
        if (semaphore.second == false)
        {
            semaphore.second = true;
            return semaphore.first;
        }
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    if (m_device->vkAPI.CreateSemaphore(m_device->getVkDevice(), &semaphoreCreateInfo, nullptr, &semaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create semaphore in queue.");
    }

    m_semaphores.insert(std::make_pair(semaphore, true));

    return semaphore;
}

void VulkanSemaphorePool::release(VkSemaphore semaphore)
{
    if (!m_semaphores.contains(semaphore))
    {
        spdlog::error("thre semaphore was not created in this semaphore pool.");
        return;
    }

    m_semaphores[semaphore] = false;
}

} // namespace jipu