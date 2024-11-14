#include "vulkan_surface.h"
#include "vulkan_adapter.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanSurface::VulkanSurface(VulkanAdapter& adapter, const SurfaceDescriptor& descriptor)
    : VulkanSurface(adapter, generateVulkanSurfaceDescriptor(descriptor))
{
}

VulkanSurface::VulkanSurface(VulkanAdapter& adapter, const VulkanSurfaceDescriptor& descriptor)
    : m_adapter(adapter)
    , m_descriptor(descriptor)
{
    createSurfaceKHR();
}

VulkanSurface::~VulkanSurface()
{
    auto& vulkanAdapter = downcast(m_adapter);
    const VulkanAPI& vkAPI = vulkanAdapter.vkAPI;

    vkAPI.DestroySurfaceKHR(vulkanAdapter.getVkInstance(), m_surface, nullptr);
}

VkSurfaceKHR VulkanSurface::getVkSurface() const
{
    return m_surface;
}

// Convert Helper
ColorSpace ToColorSpace(VkColorSpaceKHR colorSpace)
{
    switch (colorSpace)
    {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
        return ColorSpace::kSRGBNonLinear;

    case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
        return ColorSpace::kSRGBLinear;

    default:
        return ColorSpace::kUndefined;
    }
}
VkColorSpaceKHR ToVkColorSpaceKHR(ColorSpace colorSpace)
{
    switch (colorSpace)
    {
    case ColorSpace::kSRGBNonLinear:
        return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    case ColorSpace::kSRGBLinear:
        return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;

    case ColorSpace::kUndefined:
    default:
        spdlog::error("color space is undefined. use srgb non linear mode.");
        return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
}

PresentMode ToPresentMode(VkPresentModeKHR mode)
{
    switch (mode)
    {
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return PresentMode::kMailbox;
    case VK_PRESENT_MODE_FIFO_KHR:
        return PresentMode::kFifo;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return PresentMode::kFifoRelaxed;
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return PresentMode::kImmediate;
    default:
        return PresentMode::kUndefined;
    }
}
VkPresentModeKHR ToVkPresentModeKHR(PresentMode mode)
{
    switch (mode)
    {
    case PresentMode::kMailbox:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case PresentMode::kFifo:
        return VK_PRESENT_MODE_FIFO_KHR;
    case PresentMode::kFifoRelaxed:
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    case PresentMode::kImmediate:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    default:
        spdlog::error("Present Mode is undefined. use immediate mode.");
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
}

std::vector<CompositeAlphaFlag> ToCompositeAlphaFlags(VkCompositeAlphaFlagsKHR alphaFlags)
{
    std::vector<CompositeAlphaFlag> flags{};
    if (alphaFlags == VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        flags.push_back(CompositeAlphaFlag::kOpaque);
    }
    else if (alphaFlags == VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
    {
        flags.push_back(CompositeAlphaFlag::kPreMultiplied);
    }
    else if (alphaFlags == VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
    {
        flags.push_back(CompositeAlphaFlag::kPostMultiplied);
    }
    else if (alphaFlags == VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        flags.push_back(CompositeAlphaFlag::kInherit);
    }

    return flags;
}

}; // namespace jipu
