#include "vulkan_driver.h"
#include "vulkan_surface.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

void VulkanSurface::createSurfaceKHR()
{
    VkAndroidSurfaceCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .window = static_cast<ANativeWindow*>(m_windowHandle)
    };

    VulkanDriver* driver = downcast(m_driver);
    VkResult result = driver->vkAPI.CreateAndroidSurfaceKHR(driver->getVkInstance(),
                                                            &createInfo, nullptr,
                                                            &m_surface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create VkSurfaceKHR.: {}", static_cast<int32_t>(result)));
    }
}

} // namespace jipu
