#pragma once

#include <vector>

#include "vulkan_api.h"

#include "vulkan_command_recorder.h"

namespace jipu
{

enum class SubmitType
{
    kNone,
    kRender,
    kCompute,
    kTransfer,
    kPresent,
};

struct VulkanSubmit
{
    struct Info
    {
        SubmitType type = SubmitType::kNone;

        std::vector<VkCommandBuffer> commandBuffers{};
        std::vector<VkSemaphore> signalSemaphores{};
        std::vector<VkSemaphore> waitSemaphores{};
        std::vector<VkPipelineStageFlags> waitStages{};
    } info{};

    struct Object
    {
        struct Resource
        {
            std::unordered_set<VkBuffer> buffers{};
            std::unordered_set<VkImage> images{};
        };

        std::unordered_set<VkImageView> imageViews{};
        std::unordered_set<VkSampler> samplers{};
        std::unordered_set<VkPipeline> pipelines{};
        std::unordered_set<VkPipelineLayout> pipelineLayouts{};
        std::unordered_set<VkDescriptorSet> descriptorSet{};
        std::unordered_set<VkDescriptorSetLayout> descriptorSetLayouts{};
        std::unordered_set<VkFramebuffer> framebuffers{};
        std::unordered_set<VkRenderPass> renderPasses{};
        Resource srcResource{};
        Resource dstResource{};
    } object{};

    void add(VkCommandBuffer commandBuffer);
    void addSignalSemaphore(const std::vector<VkSemaphore>& semaphores);
    void addWaitSemaphore(const std::vector<VkSemaphore>& semaphores, const std::vector<VkPipelineStageFlags>& stage);

    void add(VkImageView imageView);
    void add(VkSampler sampler);
    void add(VkPipeline pipeline);
    void add(VkPipelineLayout pipelineLayout);
    void add(VkDescriptorSet descriptorSet);
    void add(VkDescriptorSetLayout descriptorSetLayout);
    void add(VkFramebuffer framebuffer);
    void add(VkRenderPass renderPass);

    void addSrcBuffer(VkBuffer buffer);
    void addSrcImage(VkImage image);
    void addDstBuffer(VkBuffer buffer);
    void addDstImage(VkImage image);
};

class VulkanDevice;
class VulkanSubmitContext final
{
public:
    VulkanSubmitContext() = default;
    ~VulkanSubmitContext() = default;

public:
    static VulkanSubmitContext create(VulkanDevice* device, const std::vector<VulkanCommandRecordResult>& results);

public:
    const std::vector<VulkanSubmit>& getSubmits() const;
    std::vector<VulkanSubmit::Info> getSubmitInfos() const;

private:
    std::vector<VulkanSubmit> m_submits{};
};

} // namespace jipu