#include "vulkan_api.h"
#include "vulkan_instance.h"
#include "vulkan_surface.h"

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace jipu
{

#if defined(VK_USE_PLATFORM_METAL_EXT)

VulkanSurfaceDescriptor generateVulkanSurfaceDescriptor(const SurfaceDescriptor& descriptor)
{
    @autoreleasepool
    {
        VulkanSurfaceDescriptor vkdescriptor{};

        vkdescriptor.layer = (__bridge CAMetalLayer*)descriptor.windowHandle;

        return vkdescriptor;
    }
}

void VulkanSurface::createSurfaceKHR()
{
    VkMetalSurfaceCreateInfoEXT surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    surfaceCreateInfo.pLayer = m_descriptor.layer;

    VkResult result = m_instance.vkAPI.CreateMetalSurfaceEXT(m_instance.getVkInstance(), &surfaceCreateInfo, nullptr, &m_surface);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create VkSurfaceKHR.: {}", static_cast<int32_t>(result)));
    }
}
#elif defined(VK_USE_PLATFORM_MACOS_MVK)

// VulkanSurfaceDescriptor generateVulkanSurfaceDescriptor(const SurfaceDescriptor& descriptor)
// {
//     @autoreleasepool
//     {
//         VulkanSurfaceDescriptor vkdescriptor{};

//         NSView* nsView = (__bridge NSView*)descriptor.windowHandle;
//         if (nsView == nil)
//         {
//             throw std::runtime_error(fmt::format("[{}] Failed to get NSView.", __func__));
//         }
//         NSBundle* bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"];
//         CAMetalLayer* layer = [[bundle classNamed:@"CAMetalLayer"] layer];

//         [nsView setLayer:layer];

//         vkdescriptor.view = (__bridge void*)nsView;

//         return vkdescriptor;
//     }
// }

// void VulkanSurface::createSurfaceKHR()
// {
//     @autoreleasepool
//     {
//         NSView* nsView = (__bridge NSView*)m_descriptor.windowHandle;
//         if (nsView == nil)
//         {
//             throw std::runtime_error(fmt::format("[{}] Failed to get NSView.", __func__));
//         }
//         NSBundle* bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"];
//         CAMetalLayer* layer = [[bundle classNamed:@"CAMetalLayer"] layer];

//         [nsView setLayer:layer];

//         VkMacOSSurfaceCreateInfoMVK createInfo{};
//         createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
//         createInfo.pView = (__bridge void*)nsView;

//         VulkanInstance& instance = downcast(m_instance);
//         const VulkanAPI& vkAPI = instance.vkAPI;
//         if (vkAPI.CreateMacOSSurfaceMVK == nullptr)
//         {
//             throw std::runtime_error("vkCreateMacOSSurfaceMVK is nullptr.");
//         }

//         VkResult result = vkAPI.CreateMacOSSurfaceMVK(instance.getVkInstance(), &createInfo, nullptr, &m_surface);

//         if (result != VK_SUCCESS)
//         {
//             throw std::runtime_error(fmt::format("Failed to create VkSurfaceKHR.: {}", static_cast<int32_t>(result)));
//         }
//     }
// }

#endif

} // namespace jipu
