#pragma once

#include "jipu/surface.h"
#include "utils/cast.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include <vector>

namespace jipu
{

struct VulkanSurfaceInfo
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
    std::vector<bool> supportedQueueFamilies; // TODO: remove or not.
};

class VulkanDriver;
class VULKAN_EXPORT VulkanSurface : public Surface
{
public:
    VulkanSurface() = delete;
    VulkanSurface(VulkanDriver* driver, const SurfaceDescriptor& descriptor);
    ~VulkanSurface() override;

    VkSurfaceKHR getSurfaceKHR() const;
    VulkanSurfaceInfo gatherSurfaceInfo(VkPhysicalDevice physicalDevice) const;

private:
    void createSurfaceKHR();

private:
    VulkanDriver* m_driver = nullptr;

    const SurfaceDescriptor m_descriptor{};

private:
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VulkanSurfaceInfo m_surfaceInfo{};
};

DOWN_CAST(VulkanSurface, Surface);

}; // namespace jipu
