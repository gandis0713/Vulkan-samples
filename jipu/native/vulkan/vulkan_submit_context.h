#pragma once

#include <vector>

#include "vulkan_api.h"
#include "vulkan_command_recorder.h"
#include "vulkan_resource.h"

namespace jipu
{

enum class SubmitType
{
    kNone,
    kGraphics,
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

        uint32_t swapchainIndex = 0; // for presnet submit type
    } info{};

    struct Object
    {
        struct Resource
        {
            std::unordered_map<VkBuffer, VulkanMemory> buffers{};
            std::unordered_map<VkImage, VulkanMemory> images{};
        };

        std::unordered_set<VkImageView> imageViews{};
        std::unordered_set<VkSampler> samplers{};
        std::unordered_set<VkPipeline> pipelines{};
        std::unordered_set<VkPipelineLayout> pipelineLayouts{};
        std::unordered_set<VkDescriptorSet> descriptorSet{};
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
    void add(VkFramebuffer framebuffer);
    void add(VkRenderPass renderPass);

    void addSrcBuffer(VulkanBufferResource buffer);
    void addSrcImage(VulkanTextureResource image);
    void addDstBuffer(VulkanBufferResource buffer);
    void addDstImage(VulkanTextureResource image);

    void add(CopyBufferToBufferCommand* command);
    void add(CopyBufferToTextureCommand* command);
    void add(CopyTextureToBufferCommand* command);
    void add(CopyTextureToTextureCommand* command);
    void add(SetComputePipelineCommand* command);
    void addComputeBindGroup(SetBindGroupCommand* command);
    void add(BeginRenderPassCommand* command);
    void addRenderBindGroup(SetBindGroupCommand* command);
    void add(SetRenderPipelineCommand* command);
    void add(SetVertexBufferCommand* command);
    void add(SetIndexBufferCommand* command);
    void add(ExecuteBundleCommand* command);
};

class VulkanDevice;
class VulkanSubmitContext final
{
public:
    VulkanSubmitContext() = default;
    ~VulkanSubmitContext() = default;

public:
    static VulkanSubmitContext create(VulkanDevice* device, const std::vector<CommandBuffer*>& commandBuffers);

public:
    const std::vector<VulkanSubmit>& getSubmits() const;
    std::vector<VulkanSubmit::Info> getSubmitInfos() const;
    std::vector<VulkanSubmit::Object> getSubmitObjects() const;

private:
    std::vector<VulkanSubmit> m_submits{};
};

} // namespace jipu