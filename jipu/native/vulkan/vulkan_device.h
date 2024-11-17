#pragma once

#include "device.h"
#include "jipu/common/cast.h"

#include "vulkan_api.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_command_buffer.h"
#include "vulkan_command_encoder.h"
#include "vulkan_command_pool.h"
#include "vulkan_export.h"
#include "vulkan_fence_pool.h"
#include "vulkan_framebuffer.h"
#include "vulkan_inflight_context.h"
#include "vulkan_object_manager.h"
#include "vulkan_pipeline.h"
#include "vulkan_pipeline_layout.h"
#include "vulkan_render_pass.h"
#include "vulkan_resource_allocator.h"
#include "vulkan_semaphore_pool.h"
#include "vulkan_swapchain.h"
#include "vulkan_texture.h"

#include <memory>
#include <unordered_set>
#include <vector>

namespace jipu
{

class VulkanPhysicalDevice;
class VULKAN_EXPORT VulkanDevice : public Device
{
public:
    VulkanDevice() = delete;
    VulkanDevice(VulkanPhysicalDevice& physicalDevice, const DeviceDescriptor& descriptor);
    ~VulkanDevice() override;

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

public:
    std::unique_ptr<Buffer> createBuffer(const BufferDescriptor& descriptor) override;
    std::unique_ptr<BindGroup> createBindGroup(const BindGroupDescriptor& descriptor) override;                   // TODO: get from cache or create.
    std::unique_ptr<BindGroupLayout> createBindGroupLayout(const BindGroupLayoutDescriptor& descriptor) override; // TODO: get from cache or create.
    std::unique_ptr<PipelineLayout> createPipelineLayout(const PipelineLayoutDescriptor& descriptor) override;    // TODO: get from cache or create.
    std::unique_ptr<QuerySet> createQuerySet(const QuerySetDescriptor& descriptor) override;
    std::unique_ptr<Queue> createQueue(const QueueDescriptor& descriptor) override;
    std::unique_ptr<ComputePipeline> createComputePipeline(const ComputePipelineDescriptor& descriptor) override; // TODO: get from cache or create.
    std::unique_ptr<RenderPipeline> createRenderPipeline(const RenderPipelineDescriptor& descriptor) override;    // TODO: get from cache or create.
    std::unique_ptr<Sampler> createSampler(const SamplerDescriptor& descriptor) override;
    std::unique_ptr<ShaderModule> createShaderModule(const ShaderModuleDescriptor& descriptor) override; // TODO: get from cache or create.
    std::unique_ptr<Swapchain> createSwapchain(const SwapchainDescriptor& descriptor) override;
    std::unique_ptr<Texture> createTexture(const TextureDescriptor& descriptor) override;
    std::unique_ptr<CommandEncoder> createCommandEncoder(const CommandEncoderDescriptor& descriptor) override;

public:
    std::unique_ptr<RenderPipeline> createRenderPipeline(const VulkanRenderPipelineDescriptor& descriptor);
    std::unique_ptr<BindGroupLayout> createBindGroupLayout(const VulkanBindGroupLayoutDescriptor& descriptor);
    std::unique_ptr<Texture> createTexture(const VulkanTextureDescriptor& descriptor);
    std::unique_ptr<Swapchain> createSwapchain(const VulkanSwapchainDescriptor& descriptor);

public:
    VulkanRenderPass* getRenderPass(const VulkanRenderPassDescriptor& descriptor);
    VulkanFramebuffer* getFrameBuffer(const VulkanFramebufferDescriptor& descriptor);
    VulkanResourceAllocator& getResourceAllocator();

public:
    VulkanPhysicalDevice& getPhysicalDevice() const;
    VulkanSemaphorePool* getSemaphorePool();
    VulkanFencePool* getFencePool();
    VulkanRenderPassCache* getRenderPassCache();
    VulkanFramebufferCache* getFramebufferCache();
    VulkanCommandPool* getCommandPool();
    VulkanInflightContext* getInflightContext();
    VulkanObjectManager* getObjectManager();

public:
    VkDevice getVkDevice() const;
    VkPhysicalDevice getVkPhysicalDevice() const;
    VkDescriptorPool getVkDescriptorPool();
    const std::vector<VkQueueFamilyProperties>& getActivatedQueueFamilies() const;

public:
    VulkanAPI vkAPI{};

private:
    void createDevice();
    const std::vector<const char*> getRequiredDeviceExtensions();
    void createPools();

private:
    VulkanPhysicalDevice& m_physicalDevice;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::unique_ptr<VulkanSemaphorePool> m_semaphorePool = nullptr;
    std::unique_ptr<VulkanFencePool> m_fencePool = nullptr;
    std::unique_ptr<VulkanCommandPool> m_commandBufferPool = nullptr;

    VulkanRenderPassCache m_renderPassCache;
    VulkanFramebufferCache m_frameBufferCache;
    std::unique_ptr<VulkanResourceAllocator> m_resourceAllocator = nullptr;
    std::unique_ptr<VulkanInflightContext> m_inflightContext = nullptr;

    std::unique_ptr<VulkanObjectManager> m_objectManager = nullptr;

    std::vector<VkQueueFamilyProperties> m_queueFamilies{};
};

DOWN_CAST(VulkanDevice, Device);

} // namespace jipu
