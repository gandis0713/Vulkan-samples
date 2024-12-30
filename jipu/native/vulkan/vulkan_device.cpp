#include "vulkan_device.h"

#include "vulkan_adapter.h"
#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_physical_device.h"
#include "vulkan_query_set.h"
#include "vulkan_queue.h"
#include "vulkan_render_bundle_encoder.h"
#include "vulkan_sampler.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanDevice::VulkanDevice(VulkanPhysicalDevice* physicalDevice, const DeviceDescriptor& descriptor)
    : vkAPI(downcast(physicalDevice->getAdapter())->vkAPI)
    , m_physicalDevice(physicalDevice)
{
    createDevice();

    const VulkanPhysicalDeviceInfo& info = physicalDevice->getVulkanPhysicalDeviceInfo();
    const VulkanDeviceKnobs& deviceKnobs = static_cast<const VulkanDeviceKnobs&>(info);
    if (!vkAPI.loadDeviceProcs(m_device, deviceKnobs))
    {
        throw std::runtime_error("Failed to load device procs.");
    }

    m_renderPassCache = std::make_shared<VulkanRenderPassCache>(this);
    m_frameBufferCache = std::make_shared<VulkanFramebufferCache>(this);

    VulkanResourceAllocatorDescriptor allocatorDescriptor{};
    m_resourceAllocator = std::make_unique<VulkanResourceAllocator>(this, allocatorDescriptor);

    m_inflightObjects = std::make_unique<VulkanInflightObjects>(this);

    m_deleter = VulkanDeleter::create(this);

    createPools();
}

VulkanDevice::~VulkanDevice()
{
    vkAPI.DeviceWaitIdle(m_device);

    m_frameBufferCache->clear();
    m_renderPassCache->clear();

    m_inflightObjects->clearAll(); // TODO: fix me
    m_deleter.reset();
    m_inflightObjects.reset();

    m_resourceAllocator.reset();

    m_descriptorPool.reset();
    m_commandBufferPool.reset();
    m_semaphorePool.reset();
    m_fencePool.reset();

    vkAPI.DestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
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

std::unique_ptr<RenderBundleEncoder> VulkanDevice::createRenderBundleEncoder(const RenderBundleEncoderDescriptor& descriptor)
{
    return VulkanRenderBundleEncoder::create(this, descriptor);
}

VulkanPhysicalDevice* VulkanDevice::getPhysicalDevice() const
{
    return m_physicalDevice;
}

std::shared_ptr<VulkanRenderPass> VulkanDevice::getRenderPass(const VulkanRenderPassDescriptor& descriptor)
{
    return m_renderPassCache->getRenderPass(descriptor);
}

std::shared_ptr<VulkanFramebuffer> VulkanDevice::getFrameBuffer(const VulkanFramebufferDescriptor& descriptor)
{
    return m_frameBufferCache->getFrameBuffer(descriptor);
}

std::shared_ptr<VulkanResourceAllocator> VulkanDevice::getResourceAllocator()
{
    return m_resourceAllocator;
}

std::shared_ptr<VulkanSemaphorePool> VulkanDevice::getSemaphorePool()
{
    return m_semaphorePool;
}

std::shared_ptr<VulkanFencePool> VulkanDevice::getFencePool()
{
    return m_fencePool;
}

std::shared_ptr<VulkanDescriptorPool> VulkanDevice::getDescriptorPool()
{
    return m_descriptorPool;
}

std::shared_ptr<VulkanRenderPassCache> VulkanDevice::getRenderPassCache()
{
    return m_renderPassCache;
}

std::shared_ptr<VulkanFramebufferCache> VulkanDevice::getFramebufferCache()
{
    return m_frameBufferCache;
}

std::shared_ptr<VulkanCommandPool> VulkanDevice::getCommandPool()
{
    return m_commandBufferPool;
}

std::shared_ptr<VulkanInflightObjects> VulkanDevice::getInflightObjects()
{
    return m_inflightObjects;
}

std::shared_ptr<VulkanDeleter> VulkanDevice::getDeleter()
{
    return m_deleter;
}

VkDevice VulkanDevice::getVkDevice() const
{
    return m_device;
}

VkPhysicalDevice VulkanDevice::getVkPhysicalDevice() const
{
    return m_physicalDevice->getVkPhysicalDevice();
}

const std::vector<VkQueueFamilyProperties>& VulkanDevice::getActivatedQueueFamilies() const
{
    return m_queueFamilies;
}

void VulkanDevice::createDevice()
{
    const VulkanPhysicalDeviceInfo& info = m_physicalDevice->getVulkanPhysicalDeviceInfo();

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

    // do not use layer for device. because it is deprecated.
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &m_physicalDevice->getVulkanPhysicalDeviceInfo().physicalDeviceFeatures;

    std::vector<const char*> requiredDeviceExtensions = getRequiredDeviceExtensions();

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    VkPhysicalDevice physicalDevice = m_physicalDevice->getVkPhysicalDevice();
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
    if (m_physicalDevice->getVulkanPhysicalDeviceInfo().portabilitySubset)
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
    m_descriptorPool = std::make_unique<VulkanDescriptorPool>(this);

    // command buffer pool
    {
        m_commandBufferPool = VulkanCommandPool::create(this);
    }
}

} // namespace jipu
