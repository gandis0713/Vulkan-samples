#include "vulkan_device.h"

#include "vulkan_adapter.h"
#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_framebuffer.h"
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
    : vkAPI(downcast(physicalDevice.getAdapter())->vkAPI)
    , m_physicalDevice(physicalDevice)
    , m_renderPassCache(this)
    , m_frameBufferCache(this)
{
    createDevice();

    const VulkanPhysicalDeviceInfo& info = physicalDevice.getVulkanPhysicalDeviceInfo();
    const VulkanDeviceKnobs& deviceKnobs = static_cast<const VulkanDeviceKnobs&>(info);
    if (!vkAPI.loadDeviceProcs(m_device, deviceKnobs))
    {
        throw std::runtime_error("Failed to load device procs.");
    }

    VulkanResourceAllocatorDescriptor allocatorDescriptor{};
    m_resourceAllocator = std::make_unique<VulkanResourceAllocator>(this, allocatorDescriptor);

    m_inflightObjects = std::make_unique<VulkanInflightObjects>(this);

    m_objectManager = VulkanObjectManager::create(this);

    createPools();
}

VulkanDevice::~VulkanDevice()
{
    vkAPI.DeviceWaitIdle(m_device);

    m_frameBufferCache.clear();
    m_renderPassCache.clear();

    m_objectManager.reset();

    m_inflightObjects.reset();

    vkAPI.DestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    m_resourceAllocator.reset();

    m_commandBufferPool.reset();
    m_semaphorePool.reset();
    m_fencePool.reset();

    vkAPI.DestroyDevice(m_device, nullptr);
}

std::unique_ptr<Buffer> VulkanDevice::createBuffer(const BufferDescriptor& descriptor)
{
    return std::make_unique<VulkanBuffer>(this, descriptor);
}

std::unique_ptr<BindGroup> VulkanDevice::createBindGroup(const BindGroupDescriptor& descriptor)
{
    return std::make_unique<VulkanBindGroup>(this, descriptor);
}

std::unique_ptr<BindGroupLayout> VulkanDevice::createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor)
{
    return std::make_unique<VulkanBindGroupLayout>(this, descriptor);
}

std::unique_ptr<PipelineLayout> VulkanDevice::createPipelineLayout(const PipelineLayoutDescriptor& descriptor)
{
    return std::make_unique<VulkanPipelineLayout>(this, descriptor);
}

std::unique_ptr<ComputePipeline> VulkanDevice::createComputePipeline(const ComputePipelineDescriptor& descriptor)
{
    return std::make_unique<VulkanComputePipeline>(this, descriptor);
}

std::unique_ptr<RenderPipeline> VulkanDevice::createRenderPipeline(const RenderPipelineDescriptor& descriptor)
{
    return std::make_unique<VulkanRenderPipeline>(this, descriptor);
}

std::unique_ptr<QuerySet> VulkanDevice::createQuerySet(const QuerySetDescriptor& descriptor)
{
    return std::make_unique<VulkanQuerySet>(this, descriptor);
}

std::unique_ptr<Queue> VulkanDevice::createQueue(const QueueDescriptor& descriptor)
{
    return std::make_unique<VulkanQueue>(this, descriptor);
}

std::unique_ptr<Sampler> VulkanDevice::createSampler(const SamplerDescriptor& descriptor)
{
    return std::make_unique<VulkanSampler>(this, descriptor);
}

std::unique_ptr<ShaderModule> VulkanDevice::createShaderModule(const ShaderModuleDescriptor& descriptor)
{
    return std::make_unique<VulkanShaderModule>(this, descriptor);
}

std::unique_ptr<Swapchain> VulkanDevice::createSwapchain(const SwapchainDescriptor& descriptor)
{
    return std::make_unique<VulkanSwapchain>(this, descriptor);
}

std::unique_ptr<Texture> VulkanDevice::createTexture(const TextureDescriptor& descriptor)
{
    return std::make_unique<VulkanTexture>(this, descriptor);
}

std::unique_ptr<RenderPipeline> VulkanDevice::createRenderPipeline(const VulkanRenderPipelineDescriptor& descriptor)
{
    return std::make_unique<VulkanRenderPipeline>(this, descriptor);
}

std::unique_ptr<BindGroupLayout> VulkanDevice::createBindGroupLayout(const VulkanBindGroupLayoutDescriptor& descriptor)
{
    return std::make_unique<VulkanBindGroupLayout>(this, descriptor);
}

std::unique_ptr<Texture> VulkanDevice::createTexture(const VulkanTextureDescriptor& descriptor)
{
    return std::make_unique<VulkanTexture>(this, descriptor);
}

std::unique_ptr<Swapchain> VulkanDevice::createSwapchain(const VulkanSwapchainDescriptor& descriptor)
{
    return std::make_unique<VulkanSwapchain>(this, descriptor);
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

VulkanInflightObjects* VulkanDevice::getInflightObjects()
{
    return m_inflightObjects.get();
}

VulkanObjectManager* VulkanDevice::getObjectManager()
{
    return m_objectManager.get();
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
        const uint32_t maxSets = 32; // TODO: set correct max value.
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

        uint32_t kDescriptorSetUniformBufferCount = 32;
        if (devicePropertyLimists.maxDescriptorSetUniformBuffers < kDescriptorSetUniformBufferCount)
            kDescriptorSetUniformBufferCount = devicePropertyLimists.maxDescriptorSetUniformBuffers;

        uint32_t kDescriptorSetUniformBufferDynamicCount = 32;
        if (devicePropertyLimists.maxDescriptorSetUniformBuffersDynamic < kDescriptorSetUniformBufferDynamicCount)
            kDescriptorSetUniformBufferDynamicCount = devicePropertyLimists.maxDescriptorSetUniformBuffersDynamic;

        uint32_t kDescriptorSetSamplers = 32;
        if (devicePropertyLimists.maxDescriptorSetSamplers < kDescriptorSetSamplers)
            kDescriptorSetSamplers = devicePropertyLimists.maxDescriptorSetSamplers;

        uint32_t kDescriptorSetSampledImages = 32;
        if (devicePropertyLimists.maxDescriptorSetSampledImages < kDescriptorSetSampledImages)
            kDescriptorSetSampledImages = devicePropertyLimists.maxDescriptorSetSampledImages;

        uint32_t kDescriptorSetInputAttachments = 32;
        if (devicePropertyLimists.maxDescriptorSetInputAttachments < kDescriptorSetInputAttachments)
            kDescriptorSetInputAttachments = devicePropertyLimists.maxDescriptorSetInputAttachments;

        uint32_t kDescriptorSetStorageBuffers = 32;
        if (devicePropertyLimists.maxDescriptorSetStorageBuffers < kDescriptorSetStorageBuffers)
            kDescriptorSetStorageBuffers = devicePropertyLimists.maxDescriptorSetStorageBuffers;

        uint32_t kDescriptorSetStorageBuffersDynamic = 32;
        if (devicePropertyLimists.maxDescriptorSetStorageBuffersDynamic < kDescriptorSetStorageBuffersDynamic)
            kDescriptorSetStorageBuffersDynamic = devicePropertyLimists.maxDescriptorSetStorageBuffersDynamic;

        poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kDescriptorSetUniformBufferCount };
        poolSizes[1] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, kDescriptorSetUniformBufferDynamicCount };
        poolSizes[2] = { VK_DESCRIPTOR_TYPE_SAMPLER, kDescriptorSetSamplers };
        poolSizes[3] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kDescriptorSetSampledImages };
        poolSizes[4] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kDescriptorSetSampledImages };
        poolSizes[5] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, kDescriptorSetInputAttachments };
        poolSizes[6] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kDescriptorSetStorageBuffers };
        poolSizes[7] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, kDescriptorSetStorageBuffersDynamic };

        VkResult result = vkAPI.CreateDescriptorPool(m_device, &poolCreateInfo, nullptr, &m_descriptorPool);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(fmt::format("Failed to create descriptor pool. {}", static_cast<uint32_t>(result)));
        }
    }

    return m_descriptorPool;
}

const std::vector<VkQueueFamilyProperties>& VulkanDevice::getActivatedQueueFamilies() const
{
    return m_queueFamilies;
}

void VulkanDevice::createDevice()
{
    const VulkanPhysicalDeviceInfo& info = m_physicalDevice.getVulkanPhysicalDeviceInfo();

    // Currently, only check GRAPHICS and COMPUTE. Because they imply TRANSFER. consider queue that has only TRANSFER.
    constexpr uint32_t queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;

    for (uint32_t i = 0; i < info.queueFamilyProperties.size(); ++i)
    {
        if ((info.queueFamilyProperties[i].queueFlags & queueFlags) == queueFlags)
        {
            m_queueFamilies.push_back(info.queueFamilyProperties[i]);
        }
    }

    std::vector<std::vector<float>> queuePriorities{};
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos{};

    queuePriorities.resize(m_queueFamilies.size());
    deviceQueueCreateInfos.resize(m_queueFamilies.size());

    const float queuePriority = 1.0f;
    for (auto index = 0; index < m_queueFamilies.size(); ++index)
    {
        const auto& queueFamily = m_queueFamilies[index];

        std::vector<float> curQueuePriorities(queueFamily.queueCount, queuePriority);
        queuePriorities[index] = std::move(curQueuePriorities);

        auto& deviceQueueCreateInfo = deviceQueueCreateInfos[index];
        deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.queueFamilyIndex = index;
        deviceQueueCreateInfo.queueCount = queueFamily.queueCount;
        deviceQueueCreateInfo.pQueuePriorities = queuePriorities[index].data();
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

void VulkanDevice::createPools()
{
    m_semaphorePool = std::make_unique<VulkanSemaphorePool>(this);
    m_fencePool = std::make_unique<VulkanFencePool>(this);

    // command buffer pool
    {
        m_commandBufferPool = VulkanCommandPool::create(this);
    }
}

} // namespace jipu
