#pragma once

#include "jipu/common/cast.h"
#include "render_bundle.h"
#include "vulkan_api.h"
#include "vulkan_command_recorder.h"

#include <memory>

namespace jipu
{

class VulkanRenderPass;
class VulkanFramebuffer;
struct VulkanCommandBufferInheritanceInfo
{
    VkRenderPass renderPass{ VK_NULL_HANDLE };
    VkFramebuffer framebuffer{ VK_NULL_HANDLE };
    uint32_t subpass = 0;
};

class VulkanDevice;
class VulkanRenderBundleEncoder;
class VulkanRenderBundle : public RenderBundle
{
public:
    static std::unique_ptr<RenderBundle> create(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor);

public:
    VulkanRenderBundle() = delete;
    ~VulkanRenderBundle() override = default;

    VulkanRenderBundle(const VulkanRenderBundle&) = delete;
    VulkanRenderBundle& operator=(const VulkanRenderBundle&) = delete;

public:
    const std::vector<std::unique_ptr<Command>>& getCommands() const;
    VkCommandBuffer getCommandBuffer(const VulkanCommandBufferInheritanceInfo& info);

private:
    VulkanRenderBundle(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor);

private:
    void beginRecord(const VulkanCommandBufferInheritanceInfo& info);
    void setRenderPipeline(SetRenderPipelineCommand* command);
    void setRenderBindGroup(SetBindGroupCommand* command);
    void setVertexBuffer(SetVertexBufferCommand* command);
    void setIndexBuffer(SetIndexBufferCommand* command);
    void setViewport(); // TODO: need check how to record viewport in secondary command buffer.
    void setScissor();  // TODO: need check how to record scissor in secondary command buffer.
    void draw(DrawCommand* command);
    void drawIndexed(DrawIndexedCommand* command);
    void endRecord();

private:
    [[maybe_unused]] VulkanDevice* m_device = nullptr;
    [[maybe_unused]] const RenderBundleDescriptor m_descriptor;

    CommandEncodingResult m_commandEncodingResult{};

private:
    // belows are used in recording secondary command buffer.
    struct RecodringContext
    {
        VulkanRenderPipeline* renderPipeline = nullptr;
        VulkanCommandBufferInheritanceInfo inheritanceInfo{};
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    } m_recordingContext{};

private:
    // Secondary command buffer.
    struct Functor
    {
        size_t operator()(const VulkanCommandBufferInheritanceInfo& info) const;
        bool operator()(const VulkanCommandBufferInheritanceInfo& lhs, const VulkanCommandBufferInheritanceInfo& rhs) const;
    };
    using SecondaryCommandBuffers = std::unordered_map<VulkanCommandBufferInheritanceInfo, VkCommandBuffer, Functor, Functor>;
    SecondaryCommandBuffers m_commandBuffers{};
};

DOWN_CAST(VulkanRenderBundle, RenderBundle);

} // namespace jipu