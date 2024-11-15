#pragma once

#include "jipu/common/cast.h"
#include "physical_device.h"
#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_surface.h"

namespace jipu
{

struct VulkanPhysicalDeviceInfo : VulkanDeviceKnobs
{
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    VkPhysicalDeviceProperties physicalDeviceProperties{};

    std::vector<VkQueueFamilyProperties> queueFamilyProperties{};

    std::vector<VkLayerProperties> layerProperties;
    std::vector<VkExtensionProperties> extensionProperties;

    std::vector<VkMemoryType> memoryTypes;
    std::vector<VkMemoryHeap> memoryHeaps;
};

struct VulkanPhysicalDeviceDescriptor
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};

class VulkanAdapter;
class VULKAN_EXPORT VulkanPhysicalDevice : public PhysicalDevice
{
public:
    VulkanPhysicalDevice() = delete;
    VulkanPhysicalDevice(VulkanAdapter& adapter, const VulkanPhysicalDeviceDescriptor& descriptor);
    ~VulkanPhysicalDevice() override;

    std::unique_ptr<Device> createDevice(const DeviceDescriptor& descriptor) override;

    PhysicalDeviceInfo getPhysicalDeviceInfo() const override;
    SurfaceCapabilities getSurfaceCapabilities(Surface* surface) const override;

public:
    Adapter* getAdapter() const override;

public:
    const VulkanPhysicalDeviceInfo& getVulkanPhysicalDeviceInfo() const;
    VulkanSurfaceInfo gatherSurfaceInfo(VulkanSurface* surface) const;

    int findMemoryTypeIndex(VkMemoryPropertyFlags flags) const;
    bool isDepthStencilSupported(VkFormat format) const;

public:
    VkInstance getVkInstance() const;
    VkPhysicalDevice getVkPhysicalDevice() const;

private:
    void gatherPhysicalDeviceInfo();

protected:
    VulkanAdapter& m_adapter;

private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VulkanPhysicalDeviceInfo m_info{};
};

DOWN_CAST(VulkanPhysicalDevice, PhysicalDevice);

} // namespace jipu
