#include "vulkan_instance.h"
#include "vulkan_surface.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

VulkanSurfaceDescriptor generateVulkanSurfaceDescriptor(const SurfaceDescriptor& descriptor)
{
    VulkanSurfaceDescriptor vkdescriptor{};
    vkdescriptor.window = static_cast<ANativeWindow*>(descriptor.windowHandle);

    return vkdescriptor;
}

void VulkanSurface::createSurfaceKHR()
{
    VkAndroidSurfaceCreateInfoKHR createInfo{ .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                                              .pNext = nullptr,
                                              .flags = 0,
                                              .window = m_descriptor.window };

    VulkanInstance& instance = downcast(m_instance);
    VkResult result = instance.vkAPI.CreateAndroidSurfaceKHR(instance.getVkInstance(),
                                                             &createInfo, nullptr,
                                                             &m_surface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create VkSurfaceKHR.: {}", static_cast<int32_t>(result)));
    }
}

} // namespace jipu
