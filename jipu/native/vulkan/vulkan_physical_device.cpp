#include "vulkan_physical_device.h"
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "vulkan_texture.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance& instance, const VulkanPhysicalDeviceDescriptor& descriptor)
    : m_instance(instance)
{
    m_physicalDevice = descriptor.physicalDevice;

    // Gather device information.
    gatherPhysicalDeviceInfo();
}

VulkanPhysicalDevice::~VulkanPhysicalDevice()
{
    // doesn't need to destroy VkPhysicalDevice.
}

std::unique_ptr<Device> VulkanPhysicalDevice::createDevice(const DeviceDescriptor& descriptor)
{
    return std::make_unique<VulkanDevice>(*this, descriptor);
}

Instance* VulkanPhysicalDevice::getInstance() const
{
    return &m_instance;
}

PhysicalDeviceInfo VulkanPhysicalDevice::getPhysicalDeviceInfo() const
{
    PhysicalDeviceInfo info{};
    info.deviceName = m_info.physicalDeviceProperties.deviceName;
    return info;
}

SurfaceCapabilities VulkanPhysicalDevice::getSurfaceCapabilities(Surface* surface) const
{
    SurfaceCapabilities capabilities{};

    auto surfaceInfo = gatherSurfaceInfo(downcast(surface));
    for (auto format : surfaceInfo.formats)
    {
        if (isSupportedVkFormat(format.format))
            capabilities.formats.push_back(ToTextureFormat(format.format));
    }
    for (auto presentMode : surfaceInfo.presentModes)
    {
        capabilities.presentModes.push_back(ToPresentMode(presentMode));
    }
    capabilities.compositeAlphaFlags = ToCompositeAlphaFlags(surfaceInfo.capabilities.supportedCompositeAlpha);

    return capabilities;
}

VkInstance VulkanPhysicalDevice::getVkInstance() const
{
    return downcast(m_instance).getVkInstance();
}

VkPhysicalDevice VulkanPhysicalDevice::getVkPhysicalDevice() const
{
    return m_physicalDevice;
}

const VulkanPhysicalDeviceInfo& VulkanPhysicalDevice::getVulkanPhysicalDeviceInfo() const
{
    return m_info;
}

void VulkanPhysicalDevice::gatherPhysicalDeviceInfo()
{
    const VulkanAPI& vkAPI = downcast(m_instance).vkAPI;

    // Gather physical device properties and features.
    vkAPI.GetPhysicalDeviceProperties(m_physicalDevice, &m_info.physicalDeviceProperties);

    spdlog::info("Vulkan Device API Version: {}.{}.{}",
                 VK_API_VERSION_MAJOR(m_info.physicalDeviceProperties.apiVersion),
                 VK_API_VERSION_MINOR(m_info.physicalDeviceProperties.apiVersion),
                 VK_API_VERSION_PATCH(m_info.physicalDeviceProperties.apiVersion));

    // currently only support AAA.BBB.CCC.
    // TODO: support AAA.BBB.CCC.DDD for NVIDIA and AAA.BBB for intel windows
    spdlog::info("Vulkan Device Instance Version: {}.{}.{}",
                 VK_API_VERSION_MAJOR(m_info.physicalDeviceProperties.driverVersion),
                 VK_API_VERSION_MINOR(m_info.physicalDeviceProperties.driverVersion),
                 VK_API_VERSION_PATCH(m_info.physicalDeviceProperties.driverVersion));

    spdlog::info("Physical Device Id: {}", static_cast<uint32_t>(m_info.physicalDeviceProperties.deviceID));
    spdlog::info("Physical Device Name: {}", m_info.physicalDeviceProperties.deviceName);
    spdlog::info("Physical Device Type: {}", static_cast<uint32_t>(m_info.physicalDeviceProperties.deviceType));
    spdlog::info("Physical Device Vender ID: {}", static_cast<uint32_t>(m_info.physicalDeviceProperties.vendorID));

    vkAPI.GetPhysicalDeviceFeatures(m_physicalDevice, &m_info.physicalDeviceFeatures);

    // Gather device memory properties.
    {
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkAPI.GetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

        m_info.memoryTypes.assign(memoryProperties.memoryTypes, memoryProperties.memoryTypes + memoryProperties.memoryTypeCount);
        m_info.memoryHeaps.assign(memoryProperties.memoryHeaps, memoryProperties.memoryHeaps + memoryProperties.memoryHeapCount);

        for (const auto& memoryType : m_info.memoryTypes)
        {
            spdlog::info("Heap index: {}, property flags: {}", memoryType.heapIndex, static_cast<uint32_t>(memoryType.propertyFlags));
        }

        for (const auto& memoryHeap : m_info.memoryHeaps)
        {
            spdlog::info("Heap size: {}, flags: {}", memoryHeap.size, static_cast<uint32_t>(memoryHeap.flags));
        }
    }

    // Gather queue Family Properties.
    {
        uint32_t queueFamilyCount = 0;
        vkAPI.GetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

        m_info.queueFamilyProperties.resize(queueFamilyCount);
        vkAPI.GetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, m_info.queueFamilyProperties.data());

        for (const auto& queueFamilyProperty : m_info.queueFamilyProperties)
        {
            spdlog::info("queue flags: {}, queue count: {}", static_cast<uint32_t>(queueFamilyProperty.queueFlags), queueFamilyProperty.queueCount);
        }
    }

    // Gather device layer properties.
    {
        uint32_t deviceLayerCount = 0;
        VkResult result = vkAPI.EnumerateDeviceLayerProperties(m_physicalDevice, &deviceLayerCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            throw std::runtime_error(fmt::format("Failure EnumerateDeviceLayerProperties to get count. Error: {}", static_cast<int32_t>(result)));
        }

        m_info.layerProperties.resize(deviceLayerCount);
        result = vkAPI.EnumerateDeviceLayerProperties(m_physicalDevice, &deviceLayerCount, m_info.layerProperties.data());
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("Failure EnumerateDeviceLayerProperties. Error: {}", static_cast<int32_t>(result)));
        }

        for (const auto& layerProperty : m_info.layerProperties)
        {
            spdlog::info("Device Layer Name: {}", layerProperty.layerName);
        }
    }

    // Gather device extension properties.
    {
        uint32_t deviceExtensionCount = 0;
        VkResult result = vkAPI.EnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &deviceExtensionCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            throw std::runtime_error(fmt::format("Failure EnumerateDeviceExtensionProperties to get count. Error: {}", static_cast<int32_t>(result)));
        }

        m_info.extensionProperties.resize(deviceExtensionCount);
        result = vkAPI.EnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &deviceExtensionCount, m_info.extensionProperties.data());
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("Failure EnumerateDeviceExtensionProperties. Error: {}", static_cast<int32_t>(result)));
        }

        for (const auto& extensionProperty : m_info.extensionProperties)
        {
            spdlog::info("Device Extention Name: {}", extensionProperty.extensionName);

            // TODO: define "VK_KHR_portability_subset"
            if (strncmp(extensionProperty.extensionName, "VK_KHR_portability_subset", VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_info.portabilitySubset = true;
            }

            if (strncmp(extensionProperty.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_info.swapchain = true;
            }
        }
    }
}

VulkanSurfaceInfo VulkanPhysicalDevice::gatherSurfaceInfo(VulkanSurface* surface) const
{
    VulkanSurfaceInfo surfaceInfo{};

    const VulkanAPI& vkAPI = downcast(m_instance).vkAPI;
    VkResult result = vkAPI.GetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface->getVkSurface(), &surfaceInfo.capabilities);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failure GetPhysicalDeviceSurfaceCapabilitiesKHR Error: {}", static_cast<int32_t>(result)));
    }

    // Surface formats.
    {
        uint32_t surfaceFormatCount;
        result = vkAPI.GetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface->getVkSurface(), &surfaceFormatCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            throw std::runtime_error(fmt::format("Failure GetPhysicalDeviceSurfaceFormatsKHR to get count. Error: {}", static_cast<int32_t>(result)));
        }

        surfaceInfo.formats.resize(surfaceFormatCount);
        result = vkAPI.GetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface->getVkSurface(), &surfaceFormatCount, surfaceInfo.formats.data());
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("Failure GetPhysicalDeviceSurfaceFormatsKHR. Error: {}", static_cast<int32_t>(result)));
        }

        // for (const auto& format : surfaceInfo.formats)
        // {
        //     spdlog::trace("Surface format, colorspace: {}, {}",
        //                   static_cast<uint32_t>(format.format),
        //                   static_cast<uint32_t>(format.colorSpace));
        // }
    }

    // Surface present modes.
    {
        uint32_t presentModeCount;
        result = vkAPI.GetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface->getVkSurface(), &presentModeCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            throw std::runtime_error(fmt::format("Failure GetPhysicalDeviceSurfacePresentModesKHR to get count. Error: {}", static_cast<int32_t>(result)));
        }

        surfaceInfo.presentModes.resize(presentModeCount);
        result = vkAPI.GetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface->getVkSurface(), &presentModeCount, surfaceInfo.presentModes.data());
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("Failure GetPhysicalDeviceSurfacePresentModesKHR. Error: {}", static_cast<int32_t>(result)));
        }
    }

    return surfaceInfo;
}

int VulkanPhysicalDevice::findMemoryTypeIndex(VkMemoryPropertyFlags flags) const
{
    int memoryTypeIndex = -1;
    for (int i = 0u; i < m_info.memoryTypes.size(); ++i)
    {
        const auto& memoryType = m_info.memoryTypes[i];
        if ((memoryType.propertyFlags & flags) == flags)
        {
            memoryTypeIndex = i;
            break;
        }
    }

    return memoryTypeIndex;
}

bool VulkanPhysicalDevice::isDepthStencilSupported(VkFormat format) const
{
    const VulkanAPI& vkAPI = downcast(m_instance).vkAPI;

    VkFormatProperties formatProperties{};
    vkAPI.GetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProperties);

    return formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
}

} // namespace jipu
