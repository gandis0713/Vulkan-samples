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
    std::unique_ptr<RenderBundle> finish(const RenderBundleDescriptor& descriptor) override;

private:
    VulkanRenderBundleEncoder(const RenderBundleEncoderDescriptor& descriptor);

private:
    [[maybe_unused]] const RenderBundleEncoderDescriptor m_descriptor;
};

} // namespace jipu