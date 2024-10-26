#pragma once

#include "vulkan_api.h"
#include "vulkan_submit_context.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace jipu
{

struct VulkanInflightObject
{
    std::unordered_set<VkCommandBuffer> commandBuffers{};
    std::unordered_set<VkBuffer> buffers{};
    std::unordered_set<VkImage> images{};
    std::unordered_set<VkImageView> imageViews{};
    std::unordered_set<VkSemaphore> signalSemaphores{};
    // std::unordered_set<VkSemaphore> waitSemaphores{}; // not used.
    std::unordered_set<VkSampler> samplers{};
    std::unordered_set<VkPipeline> pipelines{};
    std::unordered_set<VkPipelineLayout> pipelineLayouts{};
    std::unordered_set<VkDescriptorSet> descriptorSet{};
    std::unordered_set<VkDescriptorSetLayout> descriptorSetLayouts{};
    std::unordered_set<VkFramebuffer> framebuffers{};
    std::unordered_set<VkRenderPass> renderPasses{};
};

using VulkanInflightObjects = std::unordered_map<VkFence, VulkanInflightObject>;

class VulkanDevice;
class CommandBuffer;
class VulkanInflightContext final
{
public:
    static VulkanInflightObject generate(std::vector<CommandBuffer*> commandBuffers);

public:
    VulkanInflightContext() = delete;
    explicit VulkanInflightContext(VulkanDevice* device);
    ~VulkanInflightContext();

public:
    void add(VkQueue queue, const VulkanSubmit& submit, VkFence fence);
    bool clear(VkFence fence);
    bool clear(VkQueue queue);
    void clearAll();

private:
    [[maybe_unused]] VulkanDevice* m_device = nullptr;

private:
    std::unordered_map<VkQueue, VulkanInflightObjects> m_inflights{};
};

} // namespace jipu