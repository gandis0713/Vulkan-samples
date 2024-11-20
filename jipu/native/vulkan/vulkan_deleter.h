#pragma once

#include "vulkan_api.h"
#include "vulkan_resource.h"

#include <memory>
#include <unordered_set>

namespace jipu
{

class VulkanDevice;
class VulkanDeleter final
{
public:
    static std::unique_ptr<VulkanDeleter> create(VulkanDevice* device);

public:
    VulkanDeleter() = delete;
    ~VulkanDeleter();

public:
    void safeDestroy(VkBuffer buffer, VulkanMemory memory);
    void safeDestroy(VkImage image, VulkanMemory memory);
    void safeDestroy(VkCommandBuffer commandBuffer);
    void safeDestroy(VkImageView imageView);
    void safeDestroy(VkSemaphore semaphore);
    void safeDestroy(VkSampler sampler);
    void safeDestroy(VkPipeline pipeline);
    void safeDestroy(VkPipelineLayout pipelineLayout);
    void safeDestroy(VkDescriptorSet descriptorSet);
    void safeDestroy(VkDescriptorSetLayout descriptorSetLayout);
    void safeDestroy(VkFramebuffer framebuffer);
    void safeDestroy(VkRenderPass renderPass);
    void safeDestroy(VkFence fence);

private:
    void destroy(VkBuffer buffer, VulkanMemory memory);
    void destroy(VkImage image, VulkanMemory memory);
    void destroy(VkCommandBuffer commandBuffer);
    void destroy(VkImageView imageView);
    void destroy(VkSemaphore semaphore);
    void destroy(VkSampler sampler);
    void destroy(VkPipeline pipeline);
    void destroy(VkPipelineLayout pipelineLayout);
    void destroy(VkDescriptorSet descriptorSet);
    void destroy(VkDescriptorSetLayout descriptorSetLayout);
    void destroy(VkFramebuffer framebuffer);
    void destroy(VkRenderPass renderPass);
    void destroy(VkFence fence);

private:
    VulkanDeleter(VulkanDevice* device);

private:
    VulkanDevice* m_device = nullptr;

private:
    std::unordered_map<VkBuffer, VulkanMemory> m_buffers{};
    std::unordered_map<VkImage, VulkanMemory> m_images{};
    std::unordered_set<VkCommandBuffer> m_commandBuffers{};
    std::unordered_set<VkImageView> m_imageViews{};
    std::unordered_set<VkSemaphore> m_semaphores{};
    std::unordered_set<VkSampler> m_samplers{};
    std::unordered_set<VkPipeline> m_pipelines{};
    std::unordered_set<VkPipelineLayout> m_pipelineLayouts{};
    std::unordered_set<VkDescriptorSet> m_descriptorSets{};
    std::unordered_set<VkDescriptorSetLayout> m_descriptorSetLayouts{};
    std::unordered_set<VkFramebuffer> m_framebuffers{};
    std::unordered_set<VkRenderPass> m_renderPasses{};
    std::unordered_set<VkFence> m_fences{};
};

} // namespace jipu