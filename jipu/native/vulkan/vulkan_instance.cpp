#include "vulkan_instance.h"

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

VulkanInstance::VulkanInstance(const InstanceDescriptor& descriptor) noexcept(false)
{
    initialize();
}

VulkanInstance::~VulkanInstance()
{
#ifndef NDEBUG
    if (m_debugUtilsMessenger)
    {
        vkAPI.DestroyDebugUtilsMessengerEXT(m_instance, m_debugUtilsMessenger, nullptr);
    }
#endif
    vkAPI.DestroyInstance(m_instance, nullptr);
}

void VulkanInstance::initialize() noexcept(false)
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

std::vector<std::unique_ptr<PhysicalDevice>> VulkanInstance::getPhysicalDevices()
{
    std::vector<std::unique_ptr<PhysicalDevice>> physicalDevices{};
    for (auto physicalDevice : m_physicalDevices)
    {
        VulkanPhysicalDeviceDescriptor descriptor{};
        descriptor.physicalDevice = physicalDevice;

        auto device = std::make_unique<VulkanPhysicalDevice>(*this, descriptor);
        physicalDevices.push_back(std::move(device));
    }

    return physicalDevices;
}

std::unique_ptr<Surface> VulkanInstance::createSurface(const SurfaceDescriptor& descriptor)
{
    return std::make_unique<VulkanSurface>(*this, descriptor);
}

std::unique_ptr<Surface> VulkanInstance::createSurface(const VulkanSurfaceDescriptor& descriptor)
{
    return std::make_unique<VulkanSurface>(*this, descriptor);
}

VkInstance VulkanInstance::getVkInstance() const
{
    return m_instance;
}

const std::vector<VkPhysicalDevice>& VulkanInstance::getVkPhysicalDevices() const
{
    return m_physicalDevices;
}

VkPhysicalDevice VulkanInstance::getVkPhysicalDevice(uint32_t index) const
{
    assert(index < m_physicalDevices.size());

    return m_physicalDevices[index];
}

const VulkanInstanceInfo& VulkanInstance::getInstanceInfo() const
{
    return m_instanceInfo;
}

void VulkanInstance::createInstance() noexcept(false)
{
    // Application Information.
    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = m_instanceInfo.apiVersion;

    spdlog::info("Required Vulkan API Version in Application: {}.{}.{}",
                 VK_API_VERSION_MAJOR(applicationInfo.apiVersion),
                 VK_API_VERSION_MINOR(applicationInfo.apiVersion),
                 VK_API_VERSION_PATCH(applicationInfo.apiVersion));

    // Create Vulkan instance.
    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

#if VK_HEADER_VERSION >= 216
    if (m_instanceInfo.portabilityEnum)
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

    VkResult result = vkAPI.CreateInstance(&instanceCreateInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create VkInstance: {}", static_cast<int32_t>(result)));
    }

    const VulkanInstanceKnobs& instanceKnobs = static_cast<const VulkanInstanceKnobs&>(m_instanceInfo);
    if (!vkAPI.loadInstanceProcs(m_instance, instanceKnobs))
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
        result = vkAPI.CreateDebugUtilsMessengerEXT(m_instance, &debugUtilsMessengerCreateInfo, nullptr, &m_debugUtilsMessenger);
        if (result != VK_SUCCESS)
        {
            spdlog::error("Failed to create debug util messager: {}", static_cast<int32_t>(result));
        }
    }
#endif
}

void VulkanInstance::gatherPhysicalDevices() noexcept(false)
{
    uint32_t physicalDeviceCount = 0;
    vkAPI.EnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

    spdlog::info("Physical Device Count: {}", physicalDeviceCount);
    if (physicalDeviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    m_physicalDevices.resize(physicalDeviceCount);
    vkAPI.EnumeratePhysicalDevices(m_instance, &physicalDeviceCount, m_physicalDevices.data());
    if (m_physicalDevices.empty())
    {
        throw std::runtime_error("There is no physical device.");
    }
}

void VulkanInstance::gatherInstanceInfo()
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

        m_instanceInfo.layerProperties.resize(instanceLayerCount);
        result = vkAPI.EnumerateInstanceLayerProperties(&instanceLayerCount, m_instanceInfo.layerProperties.data());
        if (result != VK_SUCCESS)
        {
            spdlog::error("Failed to enumerate instance layer properties. {}", static_cast<int32_t>(result));
            return;
        }

        for (const auto& layerProperty : m_instanceInfo.layerProperties)
        {
            // TODO: set instance knobs for layer
            spdlog::info("Instance Layer Name: {}", layerProperty.layerName);
#ifndef NDEBUG
            if (strncmp(layerProperty.layerName, kLayerKhronosValidation, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.validation = true;
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

        m_instanceInfo.extensionProperties.resize(instanceExtensionCount);
        result = vkAPI.EnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, m_instanceInfo.extensionProperties.data());
        if (result != VK_SUCCESS)
        {
            spdlog::error("Failed to enumerate instance extension properties.");
            return;
        }

        for (const auto& extensionProperty : m_instanceInfo.extensionProperties)
        {
            // TODO: set instance knobs for extension
            spdlog::info("Instance Extension Name: {}, SpecVersion: {}", extensionProperty.extensionName, extensionProperty.specVersion);

            if (strncmp(extensionProperty.extensionName, kExtensionNameKhrSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.surface = true;
            }

            if (strncmp(extensionProperty.extensionName, kExtensionNameKhrAndroidSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.androidSurface = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameExtMetalSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.metalSurface = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameMvkMacosSurface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.macosSurface = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameKhrWin32Surface, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.win32Surface = true;
            }
#ifndef NDEBUG
            if (strncmp(extensionProperty.extensionName, kExtensionNameExtDebugReport, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.debugReport = true;
            }
            if (strncmp(extensionProperty.extensionName, kExtensionNameExtDebugUtils, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.debugUtils = true;
            }
#endif
#if VK_HEADER_VERSION >= 216
            if (strncmp(extensionProperty.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, VK_MAX_EXTENSION_NAME_SIZE) == 0)
            {
                m_instanceInfo.portabilityEnum = true;
            }
#endif
        }
    }
}

bool VulkanInstance::checkInstanceExtensionSupport(const std::vector<const char*> requiredInstanceExtensions)
{
    for (const auto& requiredInstanceExtension : requiredInstanceExtensions)
    {
        bool extensionFound = false;
        for (const auto& availableInstanceExtension : m_instanceInfo.extensionProperties)
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

const std::vector<const char*> VulkanInstance::getRequiredInstanceExtensions()
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
    if (m_instanceInfo.debugReport)
        requiredInstanceExtensions.push_back(kExtensionNameExtDebugReport);
    if (m_instanceInfo.debugUtils)
        requiredInstanceExtensions.push_back(kExtensionNameExtDebugUtils);
#endif

#if VK_HEADER_VERSION >= 216
    if (m_instanceInfo.portabilityEnum)
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

bool VulkanInstance::checkInstanceLayerSupport(const std::vector<const char*> requiredInstanceLayers)
{
    for (const auto& requiredInstanceLayer : requiredInstanceLayers)
    {
        bool layerFound = false;
        for (const auto& availableInstanceLayer : m_instanceInfo.layerProperties)
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

const std::vector<const char*> VulkanInstance::getRequiredInstanceLayers()
{
    std::vector<const char*> requiredInstanceLayers{};

#ifndef NDEBUG
    if (m_instanceInfo.validation)
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
