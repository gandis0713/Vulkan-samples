#pragma once

#include "render_pass_encoder.h"
#include "vulkan_api.h"
#include "vulkan_command.h"
#include "vulkan_export.h"
#include "vulkan_framebuffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_render_pass.h"

#include "jipu/common/cast.h"

namespace jipu
{

class VulkanDevice;
class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanCommandEncoder;

struct VulkanRenderPassEncoderDescriptor
{
    const void* next = nullptr;
    VulkanRenderPass* renderPass = nullptr;
    VulkanFramebuffer* framebuffer = nullptr;
    VkRect2D renderArea{};
    std::vector<VkClearValue> clearValues{};

    // TODO: convert timestampWrites for vulkan.
    QuerySet* occlusionQuerySet = nullptr;
    RenderPassTimestampWrites timestampWrites{};
};

class VULKAN_EXPORT VulkanRenderPassEncoder : public RenderPassEncoder
{
public:
    VulkanRenderPassEncoder() = delete;
    VulkanRenderPassEncoder(VulkanCommandEncoder* commandEncoder, const RenderPassEncoderDescriptor& descriptor);
    VulkanRenderPassEncoder(VulkanCommandEncoder* commandEncoder, const VulkanRenderPassEncoderDescriptor& descriptor);
    ~VulkanRenderPassEncoder() override = default;

    void setPipeline(RenderPipeline* pipeline) override;
    void setBindGroup(uint32_t index, BindGroup& bindGroup, std::vector<uint32_t> dynamicOffset = {}) override;
    void setVertexBuffer(uint32_t slot, Buffer& buffer) override;
    void setIndexBuffer(Buffer& buffer, IndexFormat format) override;
    void setViewport(float x,
                     float y,
                     float width,
                     float height,
                     float minDepth,
                     float maxDepth) override;
    void setScissor(float x,
                    float y,
                    float width,
                    float height) override;
    void setBlendConstant(const Color& color) override;

    void draw(uint32_t vertexCount,
              uint32_t instanceCount,
              uint32_t firstVertex,
              uint32_t firstInstance) override;

    void drawIndexed(uint32_t indexCount,
                     uint32_t instanceCount,
                     uint32_t indexOffset,
                     uint32_t vertexOffset,
                     uint32_t firstInstance) override;

    void beginOcclusionQuery(uint32_t queryIndex) override;
    void endOcclusionQuery() override;

    void end() override;

public:
    void nextPass();

private:
    void resetQuery();

private:
    VulkanCommandEncoder* m_commandEncoder = nullptr;
    // VulkanRenderPipeline* m_pipeline = nullptr;

    // uint32_t m_passIndex = 0;

    const VulkanRenderPassEncoderDescriptor m_descriptor{};
};
DOWN_CAST(VulkanRenderPassEncoder, RenderPassEncoder);

// Generate Helper
VulkanRenderPassDescriptor VULKAN_EXPORT generateVulkanRenderPassDescriptor(const RenderPassEncoderDescriptor& descriptor);
VulkanFramebufferDescriptor VULKAN_EXPORT generateVulkanFramebufferDescriptor(VulkanRenderPass& renderPass, const RenderPassEncoderDescriptor& descriptor);
VulkanRenderPassEncoderDescriptor VULKAN_EXPORT generateVulkanRenderPassEncoderDescriptor(VulkanDevice& device, const RenderPassEncoderDescriptor& descriptor);

// Convert Helper
VkIndexType ToVkIndexType(IndexFormat format);

} // namespace jipu