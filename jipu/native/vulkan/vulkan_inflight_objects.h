#pragma once

#include "vulkan_api.h"
#include "vulkan_resource.h"
#include "vulkan_submit_context.h"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace jipu
{

struct VulkanInflightObject
{
    std::unordered_set<VkCommandBuffer> commandBuffers{};
    std::unordered_map<VkBuffer, VulkanMemory> buffers{};
    std::unordered_map<VkImage, VulkanMemory> images{};
    std::unordered_set<VkImageView> imageViews{};
    std::unordered_set<VkSemaphore> signalSemaphores{};
    std::unordered_set<VkSemaphore> waitSemaphores{};
    std::unordered_set<VkSampler> samplers{};
    std::unordered_set<VkPipeline> pipelines{};
    std::unordered_set<VkPipelineLayout> pipelineLayouts{};
    std::unordered_set<VkDescriptorSet> descriptorSet{};
    std::unordered_set<VkDescriptorSetLayout> descriptorSetLayouts{};
    std::unordered_set<VkFramebuffer> framebuffers{};
    std::unordered_set<VkRenderPass> renderPasses{};
};

class VulkanDevice;
class CommandBuffer;
class VulkanInflightObjects final
{

public:
    using Subscribe = std::function<void(VkFence, VulkanInflightObject)>;

public:
    VulkanInflightObjects() = delete;
    explicit VulkanInflightObjects(VulkanDevice* device);
    ~VulkanInflightObjects();

public:
    void add(VkFence fence, const std::vector<VulkanSubmit>& submits);
    bool clear(VkFence fence);
    void clearAll();

    void subscribe(void* ptr, Subscribe sub);
    void unsubscribe(void* ptr);

public:
    bool isInflight(VkCommandBuffer commandBuffer) const;
    bool isInflight(VkBuffer buffer) const;
    bool isInflight(VkImage image) const;
    bool isInflight(VkImageView imageView) const;
    bool isInflight(VkSemaphore semaphore) const;
    bool isInflight(VkSampler sampler) const;
    bool isInflight(VkPipeline pipeline) const;
    bool isInflight(VkPipelineLayout pipelineLayout) const;
    bool isInflight(VkDescriptorSet descriptorSet) const;
    bool isInflight(VkDescriptorSetLayout descriptorSetLayout) const;
    bool isInflight(VkFramebuffer framebuffer) const;
    bool isInflight(VkRenderPass renderPass) const;
    bool isInflight(VkFence fence) const;

private:
    [[maybe_unused]] VulkanDevice* m_device = nullptr;

private:
    std::unordered_map<VkFence, VulkanInflightObject> m_inflightObjects{};
    std::unordered_map<void*, Subscribe> m_subs{};

    mutable std::mutex m_objectMutex{};
};

} // namespace jipu