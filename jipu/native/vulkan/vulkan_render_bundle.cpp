#include "vulkan_render_bundle.h"

namespace jipu
{

std::unique_ptr<RenderBundle> VulkanRenderBundle::create(const RenderBundleDescriptor& descriptor)
{
    return std::unique_ptr<RenderBundle>(new VulkanRenderBundle(descriptor));
}

VulkanRenderBundle::VulkanRenderBundle(const RenderBundleDescriptor& descriptor)
    : RenderBundle()
{
}

} // namespace jipu