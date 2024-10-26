#pragma once

#include "instance.h"
#include "jipu/common/cast.h"
#include "jipu/common/dylib.h"
#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_surface.h"

#include <memory>
#include <vector>

namespace jipu
{

struct VulkanInstanceInfo : VulkanInstanceKnobs
{
    std::vector<VkLayerProperties> layerProperties;
    std::vector<VkExtensionProperties> extensionProperties;
};

class VULKAN_EXPORT VulkanInstance : public Instance
{

public:
    VulkanInstance() = delete;
    VulkanInstance(const InstanceDescriptor& descriptor) noexcept(false);
    ~VulkanInstance() override;

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

public:
    std::vector<std::unique_ptr<PhysicalDevice>> getPhysicalDevices() override;
    std::unique_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) override;

public:
    std::unique_ptr<Surface> createSurface(const VulkanSurfaceDescriptor& descriptor);

public: // vulkan
    VkInstance getVkInstance() const;
    const std::vector<VkPhysicalDevice>& getVkPhysicalDevices() const;
    VkPhysicalDevice getVkPhysicalDevice(uint32_t index) const;

    const VulkanInstanceInfo& getInstanceInfo() const;

public:
    VulkanAPI vkAPI{};

private:
    void initialize() noexcept(false);
    void createInstance() noexcept(false);
    void gatherPhysicalDevices() noexcept(false);

    void gatherInstanceInfo();

    bool checkInstanceExtensionSupport(const std::vector<const char*> requiredInstanceExtensions);
    const std::vector<const char*> getRequiredInstanceExtensions();
    bool checkInstanceLayerSupport(const std::vector<const char*> requiredInstanceLayers);
    const std::vector<const char*> getRequiredInstanceLayers();

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> m_physicalDevices{};

    DyLib m_vulkanLib{};
    VulkanInstanceInfo m_instanceInfo{};
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT m_debugUtilsMessenger = VK_NULL_HANDLE;
#endif
};

DOWN_CAST(VulkanInstance, Instance);

} // namespace jipu
