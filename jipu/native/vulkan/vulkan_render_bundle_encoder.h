#pragma once

#include "render_bundle_encoder.h"

namespace jipu
{

class VulkanRenderBundleEncoder : public RenderBundleEncoder
{
public:
    static std::unique_ptr<RenderBundleEncoder> create(const RenderBundleEncoderDescriptor& descriptor);

public:
    VulkanRenderBundleEncoder() = delete;
    ~VulkanRenderBundleEncoder() override = default;

    VulkanRenderBundleEncoder(const VulkanRenderBundleEncoder&) = delete;
    VulkanRenderBundleEncoder& operator=(const VulkanRenderBundleEncoder&) = delete;

public:
    void setPipeline(RenderPipeline* pipeline) override;
    void setBindGroup(uint32_t index, BindGroup* bindGroup, std::vector<uint32_t> dynamicOffset = {}) override;
    void setVertexBuffer(uint32_t slot, Buffer* buffer) override;
    void setIndexBuffer(Buffer* buffer, IndexFormat format) override;

    void draw(uint32_t vertexCount,
              uint32_t instanceCount,
              uint32_t firstVertex,
              uint32_t firstInstance) override;

    void drawIndexed(uint32_t indexCount,
                     uint32_t instanceCount,
                     uint32_t indexOffset,
                     uint32_t vertexOffset,
                     uint32_t firstInstance) override;

    std::unique_ptr<RenderBundle> finish(const RenderBundleDescriptor& descriptor) override;

private:
    VulkanRenderBundleEncoder(const RenderBundleEncoderDescriptor& descriptor);

private:
    [[maybe_unused]] const RenderBundleEncoderDescriptor m_descriptor;
};

} // namespace jipu