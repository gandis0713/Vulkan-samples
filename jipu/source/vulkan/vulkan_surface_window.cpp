#include "vulkan_driver.h"
#include "vulkan_surface.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

VulkanSurfaceDescriptor generateVulkanSurfaceDescriptor(const SurfaceDescriptor& descriptor)
{
    VulkanSurfaceDescriptor vkdescriptor{};

    vkdescriptor.hwnd = (HWND)descriptor.windowHandle;
    vkdescriptor.hinstance = GetModuleHandle(nullptr);

    return vkdescriptor;
}

void VulkanSurface::createSurfaceKHR()
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = m_descriptor.hwnd;
    createInfo.hinstance = m_descriptor.hinstance;

    VulkanDriver& driver = downcast(m_driver);
    VkResult result = driver.vkAPI.CreateWin32SurfaceKHR(driver.getVkInstance(), &createInfo, nullptr, &m_surface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create VkSurfaceKHR.: {}", static_cast<int32_t>(result)));
    }
}

} // namespace jipu
