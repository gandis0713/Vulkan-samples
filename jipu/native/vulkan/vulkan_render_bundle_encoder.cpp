#include "vulkan_render_bundle_encoder.h"

#include "vulkan_render_bundle.h"

namespace jipu
{

std::unique_ptr<RenderBundleEncoder> VulkanRenderBundleEncoder::create(const RenderBundleEncoderDescriptor& descriptor)
{
    return std::unique_ptr<RenderBundleEncoder>(new VulkanRenderBundleEncoder(descriptor));
}

VulkanRenderBundleEncoder::VulkanRenderBundleEncoder(const RenderBundleEncoderDescriptor& descriptor)
    : RenderBundleEncoder()
    , m_descriptor(descriptor)
{
}

std::unique_ptr<RenderBundle> VulkanRenderBundleEncoder::finish(const RenderBundleDescriptor& descriptor)
{
    return VulkanRenderBundle::create(descriptor);
}

} // namespace jipu