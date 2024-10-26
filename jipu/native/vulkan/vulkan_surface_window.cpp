#include "vulkan_instance.h"
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

    VulkanInstance& instance = downcast(m_instance);
    VkResult result = instance.vkAPI.CreateWin32SurfaceKHR(instance.getVkInstance(), &createInfo, nullptr, &m_surface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create VkSurfaceKHR.: {}", static_cast<int32_t>(result)));
    }
}

} // namespace jipu
