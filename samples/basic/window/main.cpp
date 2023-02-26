
#include <vulkan/vulkan.h>
// #define GLFW_INCLUDE_VULKAN
#include "vk/context.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <vector>
namespace
{
vkt::Context context;
VkQueue graphicsQueue;
GLFWwindow* pWindow = nullptr;
VkSurfaceKHR surface;

constexpr int32_t WIDTH = 512;
constexpr int32_t HEIGHT = 512;
constexpr float graphicsQueuePriority = 1.0f;

uint32_t graphicsQueueFamilyIndex = 0xffff;

std::vector<const char*> instanceExtensionNames
{
    "VK_KHR_surface",
/*
    https://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
    https://sourceforge.net/p/predef/wiki/OperatingSystems/
*/
#if defined(__linux__)
        "VK_KHR_xcb_surface", // for glfw on linux(ubuntu)
#elif defined(_WIN64)
        "VK_KHR_win32_surface",
#else
        "VK_MVK_macos_surface", "VK_EXT_metal_surface", VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
};

std::vector<const char*> deviceExtensionNames{ "VK_KHR_swapchain" };
} // namespace

VkResult checkInstanceLayer();
VkResult checkInstanceExtension();
VkResult createInstance();
VkResult createPhysicalDevice();
VkResult createDevice();
bool createWindow();
VkResult createSurface();
VkResult checkSurfaceSupport(const uint32_t queueFamilyIndex, VkBool32& supported);

int main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::trace);
    spdlog::error("argc: {}", argc);
    spdlog::error("argv: {}", argv[0]);

    VkResult result = VK_SUCCESS;

    // check instance extension.
    result = checkInstanceExtension();
    if (result != VK_SUCCESS)
    {
        return -1;
    }

    // check instance layer.
    result = checkInstanceLayer();
    if (result != VK_SUCCESS)
    {
        return -1;
    }

    // create instance.
    result = createInstance();
    if (result != VK_SUCCESS)
    {
        return -1;
    }

    // create physical device
    result = createPhysicalDevice();
    if (result != VK_SUCCESS)
    {
        return -1;
    }

    // create device
    result = createDevice();
    if (result != VK_SUCCESS)
    {
        return -1;
    }

    vkGetDeviceQueue(context.device, graphicsQueueFamilyIndex, 0, &graphicsQueue);

    if (false == createWindow())
    {
        return -1;
    }

    result = createSurface();
    if (result != VK_SUCCESS)
    {
        return -1;
    }

    VkBool32 surfaceSupported;
    result = checkSurfaceSupport(graphicsQueueFamilyIndex, surfaceSupported);
    if (result != VK_SUCCESS)
    {
        return -1;
    }

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();
    }

    vkDestroySurfaceKHR(context.instance, surface, nullptr);
    vkDestroyDevice(context.device, nullptr);
    vkDestroyInstance(context.instance, nullptr);
    glfwDestroyWindow(pWindow);
    glfwTerminate();

    return 0;
}

VkResult checkInstanceLayer()
{
    uint32_t layerCount{ 0 };
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to get layer count [Error code : " << result << "]" << std::endl;
        return result;
    }

    std::vector<VkLayerProperties> layerProperties;
    layerProperties.resize(static_cast<std::size_t>(layerCount));

    result = vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to get layer properties [Error code : " << result << "]" << std::endl;
        return result;
    }

    for (const VkLayerProperties& p : layerProperties)
    {
        std::cout << "Layer Name             : " << p.layerName << std::endl
                  << "Spec Version           : " << p.specVersion << std::endl
                  << "Implementation Version : " << p.implementationVersion << std::endl
                  << "Description            : " << p.description << std::endl;
        std::cout << std::endl;
    }

    return result;
}

VkResult checkInstanceExtension()
{
    VkResult result = VK_SUCCESS;

    uint32_t instanceExtensionCount{ 0 };

    result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to get instance extension count [Error code : " << result << "]" << std::endl;
        return result;
    }

    std::vector<VkExtensionProperties> extensionProperties;
    extensionProperties.resize(instanceExtensionCount);

    result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, extensionProperties.data());
    for (uint32_t index = 0; index < extensionProperties.size(); ++index)
    {
        std::cout << "[Instance extension]" << std::endl
                  << "Name : " << extensionProperties[index].extensionName << std::endl
                  << "specVersion : " << extensionProperties[index].specVersion << std::endl;
    }

    return result;
}

VkResult createInstance()
{
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if defined(__APPLE__)
    #if VK_HEADER_VERSION >= 216
    instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif
#endif

    // extensions
    instanceCreateInfo.enabledExtensionCount = instanceExtensionNames.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionNames.data();

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &context.instance);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create instance [Error code : " << result << "]" << std::endl;
        return result;
    }

    return result;
}

VkResult createPhysicalDevice()
{
    uint32_t physicalDeviceCount{ 0 };
    VkResult result = vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, nullptr);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to get physical device count [Error code : " << result << "]" << std::endl;
        return result;
    }

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(static_cast<std::size_t>(physicalDeviceCount));

    result = vkEnumeratePhysicalDevices(context.instance, &physicalDeviceCount, physicalDevices.data());
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to get physical devices [Error code : " << result << "]" << std::endl;
        return result;
    }

    for (const VkPhysicalDevice& physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;

        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        std::cout << physicalDeviceProperties.deviceName << std::endl;
    }

    for (const VkPhysicalDevice& physicalDevice : physicalDevices)
    {
        uint32_t deviceExtensionCount{ 0 };

        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);

        std::cout << "deviceExtensionCount : " << deviceExtensionCount << std::endl;

        std::vector<VkExtensionProperties> deviceExtensionProperties;
        deviceExtensionProperties.resize(deviceExtensionCount);

        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceExtensionProperties.data());

        for (uint32_t index = 0; index < deviceExtensionProperties.size(); ++index)
        {
            std::cout << "[Device Extension]" << std::endl
                      << "Name    : " << deviceExtensionProperties[index].extensionName << std::endl
                      << "Version : " << deviceExtensionProperties[index].specVersion << std::endl;
        }
    }

    context.physicalDevice = physicalDevices[0];

    return result;
}

VkResult createDevice()
{
    VkResult result = VK_SUCCESS;

    uint32_t queueFamilyPropertyCount{ 0 };

    vkGetPhysicalDeviceQueueFamilyProperties(context.physicalDevice, &queueFamilyPropertyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    queueFamilyProperties.resize(queueFamilyPropertyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(context.physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    bool graphicsQueueFlag = false;
    bool computeQueueFlag = false;
    bool transferQueueFlag = false;
    bool sparseQueueFlag = false;
    for (uint32_t index = 0; index < queueFamilyProperties.size(); ++index)
    {
        std::cout << std::endl;

        graphicsQueueFlag = (queueFamilyProperties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        computeQueueFlag = (queueFamilyProperties[index].queueFlags & VK_QUEUE_COMPUTE_BIT);
        transferQueueFlag = (queueFamilyProperties[index].queueFlags & VK_QUEUE_TRANSFER_BIT);
        sparseQueueFlag = (queueFamilyProperties[index].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT);

        std::cout << std::boolalpha << "graphicsQueueFlag : " << graphicsQueueFlag << std::endl;
        std::cout << std::boolalpha << "computeQueueFlag : " << computeQueueFlag << std::endl;
        std::cout << std::boolalpha << "transferQueueFlag : " << transferQueueFlag << std::endl;
        std::cout << std::boolalpha << "sparseQueueFlag : " << sparseQueueFlag << std::endl;

        std::cout << "queueCount : " << queueFamilyProperties[index].queueCount << std::endl;

        if (queueFamilyProperties[index].queueCount < 1)
        {
            continue;
        }

        if (graphicsQueueFlag)
        {
            graphicsQueueFamilyIndex = index;
            break;
        }
    }

    if (graphicsQueueFamilyIndex == 0xffff)
    {
        std::cerr << "There is no queue family for graphics" << std::endl;
        return VK_ERROR_UNKNOWN;
    }

    VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = deviceExtensionNames.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames.data();

    result = vkCreateDevice(context.physicalDevice, &deviceCreateInfo, nullptr, &context.device);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create device [Error coce : " << result << "]" << std::endl;
    }

    return result;
}

bool createWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    if (nullptr == pWindow)
    {
        std::cerr << "Failed to create window" << std::endl;
    }

    return pWindow != nullptr;
}

VkResult createSurface()
{
    VkResult result = glfwCreateWindowSurface(context.instance, pWindow, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create surface [Error code : " << result << "]" << std::endl;
    }

    return result;
}

VkResult checkSurfaceSupport(const uint32_t queueFamilyIndex, VkBool32& supported)
{
    supported = false;

    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(context.physicalDevice, graphicsQueueFamilyIndex, surface, &supported);
    if (VK_SUCCESS != result)
    {
        std::cerr << "Failed to check surface supported [Error code : " << result << "]" << std::endl;
    }

    if (false == supported)
    {
        std::cerr << "Surface is not supported [QueueFamilyIndex : " << queueFamilyIndex << "]" << std::endl;
    }

    return result;
}
