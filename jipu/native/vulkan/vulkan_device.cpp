#include "vulkan_device.h"

#include "vulkan_binding_group.h"
#include "vulkan_binding_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_instance.h"
#include "vulkan_physical_device.h"
#include "vulkan_query_set.h"
#include "vulkan_queue.h"
#include "vulkan_sampler.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanDevice::VulkanDevice(VulkanPhysicalDevice& physicalDevice, const DeviceDescriptor& descriptor)
    : vkAPI(downcast(physicalDevice.getInstance())->vkAPI)
    , m_physicalDevice(physicalDevice)
    , m_renderPassCache(*this)
    , m_frameBufferCache(*this)
{
    const VulkanPhysicalDeviceInfo& info = physicalDevice.getVulkanPhysicalDeviceInfo();

    // GRAPHICS and COMPUTE imply TRANSFER
    constexpr uint32_t queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

    std::unordered_map<uint32_t, VkQueueFamilyProperties> queueFamilies{};
    for (uint32_t i = 0; i < info.queueFamilyProperties.size(); ++i)
    {
        if ((info.queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
        {
            queueFamilies.insert({ i, info.queueFamilyProperties[i] });
        }
    }

    createDevice(queueFamilies);

    const VulkanDeviceKnobs& deviceKnobs = static_cast<const VulkanDeviceKnobs&>(info);
    if (!vkAPI.loadDeviceProcs(m_device, deviceKnobs))
    {
        throw std::runtime_error("Failed to load device procs.");
    }

    VulkanResourceAllocatorDescriptor allocatorDescriptor{};
    m_resourceAllocator = std::make_unique<VulkanResourceAllocator>(*this, allocatorDescriptor);

    m_inflightContext = std::make_unique<VulkanInflightContext>(this);

    m_semaphorePool = std::make_unique<VulkanSemaphorePool>(this);
    m_fencePool = std::make_unique<VulkanFencePool>(this);
    m_commandBufferPool = std::make_unique<VulkanCommandPool>(this);
}

VulkanDevice::~VulkanDevice()
{
    vkAPI.DeviceWaitIdle(m_device);

    vkAPI.DestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    m_commandBufferPool.reset();
    m_semaphorePool.reset();
    m_fencePool.reset();
    m_frameBufferCache.clear();
    m_renderPassCache.clear();

    m_resourceAllocator.reset();

    vkAPI.DestroyDevice(m_device, nullptr);
}

std::unique_ptr<Buffer> VulkanDevice::createBuffer(const BufferDescriptor& descriptor)
{
    return std::make_unique<VulkanBuffer>(*this, descriptor);
}

std::unique_ptr<BindingGroup> VulkanDevice::createBindingGroup(const BindingGroupDescriptor& descriptor)
{
    return std::make_unique<VulkanBindingGroup>(*this, descriptor);
}

std::unique_ptr<BindingGroupLayout> VulkanDevice::createBindingGroupLayout(const BindingGroupLayoutDescriptor& descriptor)
{
    return std::make_unique<VulkanBindingGroupLayout>(*this, descriptor);
}

std::unique_ptr<PipelineLayout> VulkanDevice::createPipelineLayout(const PipelineLayoutDescriptor& descriptor)
{
    return std::make_unique<VulkanPipelineLayout>(*this, descriptor);
}

std::unique_ptr<ComputePipeline> VulkanDevice::createComputePipeline(const ComputePipelineDescriptor& descriptor)
{
    return std::make_unique<VulkanComputePipeline>(*this, descriptor);
}

std::unique_ptr<RenderPipeline> VulkanDevice::createRenderPipeline(const RenderPipelineDescriptor& descriptor)
{
    return std::make_unique<VulkanRenderPipeline>(*this, descriptor);
}

std::unique_ptr<QuerySet> VulkanDevice::createQuerySet(const QuerySetDescriptor& descriptor)
{
    return std::make_unique<VulkanQuerySet>(*this, descriptor);
}

std::unique_ptr<Queue> VulkanDevice::createQueue(const QueueDescriptor& descriptor)
{
    return std::make_unique<VulkanQueue>(*this, descriptor);
}

std::unique_ptr<Sampler> VulkanDevice::createSampler(const SamplerDescriptor& descriptor)
{
    return std::make_unique<VulkanSampler>(*this, descriptor);
}

std::unique_ptr<ShaderModule> VulkanDevice::createShaderModule(const ShaderModuleDescriptor& descriptor)
{
    return std::make_unique<VulkanShaderModule>(*this, descriptor);
}

std::unique_ptr<Swapchain> VulkanDevice::createSwapchain(const SwapchainDescriptor& descriptor)
{
    return std::make_unique<VulkanSwapchain>(*this, descriptor);
}

std::unique_ptr<Texture> VulkanDevice::createTexture(const TextureDescriptor& descriptor)
{
    return std::make_unique<VulkanTexture>(*this, descriptor);
}

std::unique_ptr<RenderPipeline> VulkanDevice::createRenderPipeline(const VulkanRenderPipelineDescriptor& descriptor)
{
    return std::make_unique<VulkanRenderPipeline>(*this, descriptor);
}

std::unique_ptr<BindingGroupLayout> VulkanDevice::createBindingGroupLayout(const VulkanBindingGroupLayoutDescriptor& descriptor)
{
    return std::make_unique<VulkanBindingGroupLayout>(*this, descriptor);
}

std::unique_ptr<Texture> VulkanDevice::createTexture(const VulkanTextureDescriptor& descriptor)
{
    return std::make_unique<VulkanTexture>(*this, descriptor);
}

std::unique_ptr<Swapchain> VulkanDevice::createSwapchain(const VulkanSwapchainDescriptor& descriptor)
{
    return std::make_unique<VulkanSwapchain>(*this, descriptor);
}

std::unique_ptr<CommandEncoder> VulkanDevice::createCommandEncoder(const CommandEncoderDescriptor& descriptor)
{
    return std::make_unique<VulkanCommandEncoder>(this, descriptor);
}

VulkanRenderPass* VulkanDevice::getRenderPass(const VulkanRenderPassDescriptor& descriptor)
{
    return m_renderPassCache.getRenderPass(descriptor);
}

VulkanFramebuffer* VulkanDevice::getFrameBuffer(const VulkanFramebufferDescriptor& descriptor)
{
    return m_frameBufferCache.getFrameBuffer(descriptor);
}

VulkanResourceAllocator& VulkanDevice::getResourceAllocator()
{
    return *m_resourceAllocator;
}

VulkanPhysicalDevice& VulkanDevice::getPhysicalDevice() const
{
    return m_physicalDevice;
}

VulkanSemaphorePool* VulkanDevice::getSemaphorePool()
{
    return m_semaphorePool.get();
}

VulkanFencePool* VulkanDevice::getFencePool()
{
    return m_fencePool.get();
}

VulkanRenderPassCache* VulkanDevice::getRenderPassCache()
{
    return &m_renderPassCache;
}

VulkanFramebufferCache* VulkanDevice::getFramebufferCache()
{
    return &m_frameBufferCache;
}

VulkanCommandPool* VulkanDevice::getCommandPool()
{
    return m_commandBufferPool.get();
}

VulkanInflightContext* VulkanDevice::getInflightContext()
{
    return m_inflightContext.get();
}

VkDevice VulkanDevice::getVkDevice() const
{
    return m_device;
}

VkPhysicalDevice VulkanDevice::getVkPhysicalDevice() const
{
    VulkanPhysicalDevice& vulkanPhysicalDevice = downcast(m_physicalDevice);

    return vulkanPhysicalDevice.getVkPhysicalDevice();
}

VkDescriptorPool VulkanDevice::getVkDescriptorPool()
{
    if (m_descriptorPool == VK_NULL_HANDLE)
    {
        const uint32_t maxSets = 30; // TODO: set correct max value.
        const uint64_t descriptorPoolCount = 8;
        const uint64_t maxDescriptorSetSize = descriptorPoolCount;
        std::array<VkDescriptorPoolSize, descriptorPoolCount> poolSizes;
        VkDescriptorPoolCreateInfo poolCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                   .pNext = nullptr,
                                                   .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                                                   .maxSets = maxSets,
                                                   .poolSizeCount = descriptorPoolCount,
                                                   .pPoolSizes = poolSizes.data() };

        auto& vulkanPhysicalDevice = downcast(m_physicalDevice);
        const VulkanPhysicalDeviceInfo& physicalDeviceInfo = vulkanPhysicalDevice.getVulkanPhysicalDeviceInfo();
        const VkPhysicalDeviceLimits& devicePropertyLimists = physicalDeviceInfo.physicalDeviceProperties.limits;

        poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, devicePropertyLimists.maxDescriptorSetUniformBuffers };
        poolSizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, devicePropertyLimists.maxDescriptorSetUniformBuffersDynamic };
        poolSizes[2] = { VK_DESCRIPTOR_TYPE_SAMPLER, devicePropertyLimists.maxDescriptorSetSamplers };
        poolSizes[3] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, devicePropertyLimists.maxDescriptorSetSampledImages };
        poolSizes[4] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, devicePropertyLimists.maxDescriptorSetSampledImages }; // TODO: need check
        poolSizes[5] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, devicePropertyLimists.maxDescriptorSetInputAttachments };
        poolSizes[6] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, devicePropertyLimists.maxDescriptorSetStorageBuffers };
        poolSizes[7] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, devicePropertyLimists.maxDescriptorSetStorageBuffersDynamic };

        VkResult result = vkAPI.CreateDescriptorPool(m_device, &poolCreateInfo, nullptr, &m_descriptorPool);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("Failed to create descriptor pool. {}", static_cast<uint32_t>(result)));
        }
    }

    return m_descriptorPool;
}

void VulkanDevice::createDevice(const std::unordered_map<uint32_t, VkQueueFamilyProperties>& queueFamilies)
{
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;

    float queuePriority = 1.0f;
    for (const auto& [index, queueFamily] : queueFamilies)
    {
        VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
        deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.queueFamilyIndex = index;
        deviceQueueCreateInfo.queueCount = queueFamily.queueCount;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
    }

    auto& vulkanPhysicalDevice = downcast(m_physicalDevice);

    // do not use layer for device. because it is deprecated.
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &vulkanPhysicalDevice.getVulkanPhysicalDeviceInfo().physicalDeviceFeatures;

    std::vector<const char*> requiredDeviceExtensions = getRequiredDeviceExtensions();

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    VkPhysicalDevice physicalDevice = downcast(m_physicalDevice).getVkPhysicalDevice();
    VkResult result = vkAPI.CreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("failed to create logical device. {}", static_cast<uint32_t>(result)));
    }
}

const std::vector<const char*> VulkanDevice::getRequiredDeviceExtensions()
{
    std::vector<const char*> requiredDeviceExtensions;

    requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // TODO: check extension supported.
    auto& vulkanPhysicalDevice = downcast(m_physicalDevice);
    if (vulkanPhysicalDevice.getVulkanPhysicalDeviceInfo().portabilitySubset)
    {
        // TODO: define "VK_KHR_portability_subset"
        requiredDeviceExtensions.push_back("VK_KHR_portability_subset");
    }

    spdlog::info("Required Device extensions :");
    for (const auto& extension : requiredDeviceExtensions)
    {
        spdlog::info("{}{}", '\t', extension);
    }
    return requiredDeviceExtensions;
};

} // namespace jipu
