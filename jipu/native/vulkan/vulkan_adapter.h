#pragma once

#include "jipu/common/cast.h"
#include "jipu/common/dylib.h"
#include "jipu/native/adapter.h"
#include "jipu/native/physical_device.h"
#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_surface.h"

#include <memory>
#include <vector>

namespace jipu
{

struct VulkanAdapterInfo : VulkanAdapterKnobs
{
    std::vector<VkLayerProperties> layerProperties;
    std::vector<VkExtensionProperties> extensionProperties;
};

class Instance;
class VULKAN_EXPORT VulkanAdapter : public Adapter
{

public:
    VulkanAdapter() = delete;
    VulkanAdapter(Instance* instance, const AdapterDescriptor& descriptor) noexcept(false);
    ~VulkanAdapter() override;

    VulkanAdapter(const VulkanAdapter&) = delete;
    VulkanAdapter& operator=(const VulkanAdapter&) = delete;

public:
    std::vector<std::unique_ptr<PhysicalDevice>> getPhysicalDevices() override;
    std::unique_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) override;

public:
    Instance* getInstance() const override;

public: // vulkan
    std::unique_ptr<Surface> createSurface(const VulkanSurfaceDescriptor& descriptor);

public: // vulkan
    VkInstance getVkInstance() const;
    const std::vector<VkPhysicalDevice>& getVkPhysicalDevices() const;
    VkPhysicalDevice getVkPhysicalDevice(uint32_t index) const;

    const VulkanAdapterInfo& getInstanceInfo() const;

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
    Instance* m_instance = nullptr;

private:
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> m_physicalDevices{};

    DyLib m_vulkanLib{};
    VulkanAdapterInfo m_vkInstanceInfo{};
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT m_debugUtilsMessenger = VK_NULL_HANDLE;
#endif
};

DOWN_CAST(VulkanAdapter, Adapter);

} // namespace jipu
