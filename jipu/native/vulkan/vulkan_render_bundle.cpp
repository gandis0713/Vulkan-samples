#include "vulkan_render_bundle.h"

#include "vulkan_command_pool.h"
#include "vulkan_device.h"
#include "vulkan_render_bundle_encoder.h"

#include <spdlog/spdlog.h>

namespace jipu
{

std::unique_ptr<RenderBundle> VulkanRenderBundle::create(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor)
{
    return std::unique_ptr<RenderBundle>(new VulkanRenderBundle(renderBundleEncoder, descriptor));
}

VulkanRenderBundle::VulkanRenderBundle(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor)
    : RenderBundle()
    , m_device(renderBundleEncoder->getDevice())
    , m_descriptor(descriptor)
    , m_commandEncodingResult(renderBundleEncoder->extractResult())
{
}

const std::vector<std::unique_ptr<Command>>& VulkanRenderBundle::getCommands() const
{
    return m_commandEncodingResult.commands;
}

} // namespace jipu