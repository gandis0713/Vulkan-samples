#include "vulkan_adapter.h"

#include "jipu/common/assert.h"
#include "vulkan_physical_device.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

// surface
const char kExtensionNameKhrSurface[] = "VK_KHR_surface";
const char kExtensionNameMvkMacosSurface[] = "VK_MVK_macos_surface";
const char kExtensionNameExtMetalSurface[] = "VK_EXT_metal_surface";
const char kExtensionNameKhrWin32Surface[] = "VK_KHR_win32_surface";
const char kExtensionNameKhrAndroidSurface[] = "VK_KHR_android_surface";
const char kExtensionNameKhrXcbSurface[] = "VK_KHR_xcb_surface";
// const char kExtensionNameKhrWaylandSurface[] = "VK_KHR_wayland_surface";
// const char kExtensionNameKhrXlibSurface[] = "VK_KHR_xlib_surface";

// swapchain
const char kExtensionNameKhrSwapchain[] = "VK_KHR_swapchain";

#ifndef NDEBUG
// layer
const char kLayerKhronosValidation[] = "VK_LAYER_KHRONOS_validation";
// const char kLayerKhronosSynchronization2[] = "VK_LAYER_KHRONOS_synchronization2";
// const char kLayerKhronosShaderObject[] = "VK_LAYER_KHRONOS_shader_object";
// const char kLayerKhronosProfiles[] = "VK_LAYER_KHRONOS_profiles";
// const char kLayerKhronosAPIDump[] = "VK_LAYER_LUNARG_api_dump";

// debug
const char kExtensionNameExtDebugReport[] = "VK_EXT_debug_report";
const char kExtensionNameExtDebugUtils[] = "VK_EXT_debug_utils";
#endif

namespace jipu
{

VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                           VkDebugUtilsMessageTypeFlagsEXT type,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                                           void* userData)
{
    auto message = fmt::format("[{} {}]: {}", callbackData->messageIdNumber, callbackData->pMessageIdName, callbackData->pMessage);

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        spdlog::trace(message);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        spdlog::info(message);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        spdlog::warn(message);
        assert_message(false, message);
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        spdlog::error(message);
        assert_message(false, message);
    }
    else
    {
        spdlog::debug(message);
    }

    return VK_FALSE;
}

VulkanAdapter::VulkanAdapter(Instance* instance, const AdapterDescriptor& descriptor) noexcept(false)
    : m_instance(instance)
{
    initialize();
}

VulkanAdapter::~VulkanAdapter()
{
#ifndef NDEBUG
    if (m_debugUtilsMessenger)
    {
        vkAPI.DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugUtilsMessenger, nullptr);
    }
#endif
    vkAPI.DestroyInstance(m_vkInstance, nullptr);
}

void VulkanAdapter::initialize() noexcept(false)
{
#if defined(__ANDROID__) || defined(ANDROID)
    const char vulkanLibraryName[] = "libvulkan.so";
#elif defined(__linux__)
    const char vulkanLibraryName[] = "libvulkan.so.1";
#elif defined(__APPLE__)
    const char vulkanLibraryName[] = "libvulkan.dylib";
#elif defined(WIN32)
    const char vulkanLibraryName[] = "vulkan-1.dll";
#endif

    if (!m_vulkanLib.open(vulkanLibraryName))
    {
        throw std::runtime_error(fmt::format("Failed to open vulkan library: {}", vulkanLibraryName));
    }

    if (!vkAPI.loadInstanceProcs(&m_vulkanLib))
    {
        throw std::runtime_error(fmt::format("Failed to load instance prosc in vulkan library: {}", vulkanLibraryName));
    }

    gatherInstanceInfo();

    createInstance();

    gatherPhysicalDevices();
}

std::vector<std::unique_ptr<PhysicalDevice>> VulkanAdapter::getPhysicalDevices()
{
    std::vector<std::unique_ptr<PhysicalDevice>> physicalDevices{};
    for (auto physicalDevice : m_physicalDevices)
    {
        VulkanPhysicalDeviceDescriptor descriptor{};
        descriptor.physicalDevice = physicalDevice;

        auto device = std::make_unique<VulkanPhysicalDevice>(this, descriptor);
        physicalDevices.push_back(std::move(device));
    }

    return physicalDevices;
}

std::unique_ptr<Surface> VulkanAdapter::createSurface(const SurfaceDescriptor& descriptor)
{
    return std::make_unique<VulkanSurface>(this, descriptor);
}

Instance* VulkanAdapter::getInstance() const
{
    return m_instance;
}

std::unique_ptr<Surface> VulkanAdapter::createSurface(const VulkanSurfaceDescriptor& descriptor)
{
    return std::make_unique<VulkanSurface>(this, descriptor);
}

VkInstance VulkanAdapter::getVkInstance() const
{
    return m_vkInstance;
}

const std::vector<VkPhysicalDevice>& VulkanAdapter::getVkPhysicalDevices() const
{
    return m_physicalDevices;
}

VkPhysicalDevice VulkanAdapter::getVkPhysicalDevice(uint32_t index) const
{
    assert(index < m_physicalDevices.size());

    return m_physicalDevices[index];
}

const VulkanAdapterInfo& VulkanAdapter::getInstanceInfo() const
{
    return m_vkInstanceInfo;
}

void VulkanAdapter::createInstance() noexcept(false)
{
    // Application Information.
    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = m_vkInstanceInfo.apiVersion;

    spdlog::info("Required Vulkan API Version in Application: {}.{}.{}",
                 VK_API_VERSION_MAJOR(applicationInfo.apiVersion),
                 VK_API_VERSION_MINOR(applicationInfo.apiVersion),
                 VK_API_VERSION_PATCH(applicationInfo.apiVersion));

    // Create Vulkan instance.
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

#if VK_HEADER_VERSION >= 216
    if (m_vkInstanceInfo.portabilityEnum)
    {
        instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

    const std::vector<const char*> requiredInstanceLayers = getRequiredInstanceLayers();
    if (!checkInstanceLayerSupport(requiredInstanceLayers))
    {
        throw std::runtime_error("instance layers requested, but not available!");
    }

    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = requiredInstanceLayers.data();

    const std::vector<const char*> requiredInstanceExtensions = getRequiredInstanceExtensions();
    if (!checkInstanceExtensionSupport(requiredInstanceExtensions))
    {
        throw std::runtime_error("instance extensions requested, but not available!");
    }

    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    instanceCreateInfo.pNext = nullptr;

    VkResult result = vkAPI.CreateInstance(&instanceCreateInfo, nullptr, &m_vkInstance);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create VkInstance: {}", static_cast<int32_t>(result)));
    }

    const VulkanAdapterKnobs& instanceKnobs = static_cast<const VulkanAdapterKnobs&>(m_vkInstanceInfo);
    if (!vkAPI.loadInstanceProcs(m_vkInstance, instanceKnobs))
    {
        throw std::runtime_error(fmt::format("Failed to load instance prosc."));
    }

#ifndef NDEBUG
    if (instanceKnobs.debugUtils)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
        debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugUtilsMessengerCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;
        result = vkAPI.CreateDebugUtilsMessengerEXT(m_vkInstance, &debugUtilsMessengerCreateInfo, nullptr, &m_debugUtilsMessenger);
        if (result != VK_SUCCESS)
        {
            spdlog::error("Failed to create debug util messager: {}", static_cast<int32_t>(result));
        }
    }
#endif
}

void VulkanAdapter::gatherPhysicalDevices() noexcept(false)
{
    uint32_t physicalDeviceCount = 0;
    vkAPI.EnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, nullptr);

    spdlog::info("Physical Device Count: {}", physicalDeviceCount);
    if (physicalDeviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    m_physicalDevices.resize(physicalDeviceCount);
    vkAPI.EnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, m_physicalDevices.data());
    if (m_physicalDevices.empty())
    {
        throw std::runtime_error("There is no physical device.");
    }
}

void VulkanAdapter::gatherInstanceInfo()
{
    uint32_t apiVersion = 0u;
    if (vkAPI.EnumerateInstanceVersion != nullptr)
    {
        vkAPI.EnumerateInstanceVersion(&apiVersion);
    }

    spdlog::info("Vulkan Loader API Version: {}.{}.{}",
                 VK_API_VERSION_MAJOR(apiVersion),
                 VK_API_VERSION_MINOR(apiVersion),
                 VK_API_VERSION_PATCH(apiVersion));

    // Gather instance layer properties.
    {
        uint32_t instanceLayerCount = 0;
        VkResult result = vkAPI.EnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            spdlog::error("Failed to get instance layer properties count. {}", static_cast<int32_t>(result));
            return;
        }

        m_vkInstanceInfo.layerProperties.resize(instanceLayerCount);
        result = vkAPI.EnumerateInstanceLayerProperties(&instanceLayerCount, m_vkInstanceInfo.layerProperties.data());
        if (result != VK_SUCCESS)
        {
            spdlog::error("Failed to enumerate instance layer properties. {}", static_cast<int32_t>(result));
            return;
        }

        for (const auto& layerProperty : m_vkInstanceInfo.layerProperties)
        {
            // TODO: set instance knobs for layer
            spdlog::info("Instance Layer Name: {}", layerProperty.layerName);
#ifndef NDEBUG
            if (strncmp(layerProperty.layerName, kLayerKhronosValidation, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.validation = true;
            }
#endif
        }
    }

    // Gather instance extension properties.
    {
        uint32_t instanceExtensionCount = 0;
        VkResult result = vkAPI.EnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            spdlog::error("Failed to get instance extension properties count.");
            return;
        }

        m_vkInstanceInfo.extensionProperties.resize(instanceExtensionCount);
        result = vkAPI.EnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, m_vkInstanceInfo.extensionProperties.data());
        if (result != VK_SUCCESS)
        {
            spdlog::error("Failed to enumerate instance extension properties.");
            return;
        }

        for (const auto& extensionProperty : m_vkInstanceInfo.extensionProperties)
        {
            // TODO: set instance knobs for extension
            spdlog::info("Instance Extension Name: {}, SpecVersion: {}", extensionProperty.extensionName, extensionProperty.specVersion);

            if (strncmp(extensionProperty.extensionName, kExtensionNameKhrSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.surface = true;
            }

            if (strncmp(extensionProperty.extensionName, kExtensionNameKhrAndroidSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.androidSurface = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameExtMetalSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.metalSurface = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameMvkMacosSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.macosSurface = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameKhrWin32Surface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.win32Surface = true;
            }
#ifndef NDEBUG
            if (strncmp(extensionProperty.extensionName, kExtensionNameExtDebugReport, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.debugReport = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameExtDebugUtils, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.debugUtils = true;
            }
#endif
#if VK_HEADER_VERSION >= 216
            if (strncmp(extensionProperty.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_vkInstanceInfo.portabilityEnum = true;
            }
#endif
        }
    }
}

bool VulkanAdapter::checkInstanceExtensionSupport(const std::vector<const char*> requiredInstanceExtensions)
{
    for (const auto& requiredInstanceExtension : requiredInstanceExtensions)
    {
        bool extensionFound = false;
        for (const auto& availableInstanceExtension : m_vkInstanceInfo.extensionProperties)
        {
            if (strcmp(requiredInstanceExtension, availableInstanceExtension.extensionName) == 0)
            {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound)
        {
            spdlog::error("Instance extension is not found. {}", requiredInstanceExtension);
            return false;
        }
    }

    return true;
}

const std::vector<const char*> VulkanAdapter::getRequiredInstanceExtensions()
{
    std::vector<const char*> requiredInstanceExtensions{};

    requiredInstanceExtensions.push_back(kExtensionNameKhrSurface);

#if defined(__ANDROID__) || defined(ANDROID)
    requiredInstanceExtensions.push_back(kExtensionNameKhrAndroidSurface);
#elif defined(__linux__)
    requiredInstanceExtensions.push_back(kExtensionNameKhrXcbSurface);
#elif defined(_WIN32)
    requiredInstanceExtensions.push_back(kExtensionNameKhrWin32Surface);
#elif defined(__APPLE__)
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    requiredInstanceExtensions.push_back(kExtensionNameMvkMacosSurface);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    requiredInstanceExtensions.push_back(kExtensionNameExtMetalSurface);
#endif
#endif

#ifndef NDEBUG
    if (m_vkInstanceInfo.debugReport)
        requiredInstanceExtensions.push_back(kExtensionNameExtDebugReport);
    if (m_vkInstanceInfo.debugUtils)
        requiredInstanceExtensions.push_back(kExtensionNameExtDebugUtils);
#endif

#if VK_HEADER_VERSION >= 216
    if (m_vkInstanceInfo.portabilityEnum)
    {
        requiredInstanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }
#endif

    spdlog::info("Required Instance extensions :");
    for (const auto& extension : requiredInstanceExtensions)
    {
        spdlog::info("{}{}", '\t', extension);
    }

    return requiredInstanceExtensions;
}

bool VulkanAdapter::checkInstanceLayerSupport(const std::vector<const char*> requiredInstanceLayers)
{
    for (const auto& requiredInstanceLayer : requiredInstanceLayers)
    {
        bool layerFound = false;
        for (const auto& availableInstanceLayer : m_vkInstanceInfo.layerProperties)
        {
            if (strcmp(requiredInstanceLayer, availableInstanceLayer.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

const std::vector<const char*> VulkanAdapter::getRequiredInstanceLayers()
{
    std::vector<const char*> requiredInstanceLayers{};

#ifndef NDEBUG
    if (m_vkInstanceInfo.validation)
        requiredInstanceLayers.push_back(kLayerKhronosValidation);
#endif

    spdlog::info("Required Instance layers :");
    for (const auto& layer : requiredInstanceLayers)
    {
        spdlog::info("{}{}", '\t', layer);
    }

    return requiredInstanceLayers;
}

} // namespace jipu
