#include "vulkan_render_bundle_encoder.h"

#include "vulkan_device.h"
#include "vulkan_render_bundle.h"

namespace jipu
{

std::unique_ptr<RenderBundleEncoder> VulkanRenderBundleEncoder::create(VulkanDevice* device, const RenderBundleEncoderDescriptor& descriptor)
{
    return std::unique_ptr<RenderBundleEncoder>(new VulkanRenderBundleEncoder(device, descriptor));
}

VulkanRenderBundleEncoder::VulkanRenderBundleEncoder(VulkanDevice* device, const RenderBundleEncoderDescriptor& descriptor)
    : RenderBundleEncoder()
    , m_device(device)
    , m_descriptor(descriptor)
{
}

void VulkanRenderBundleEncoder::setPipeline(RenderPipeline* pipeline)
{
}

void VulkanRenderBundleEncoder::setBindGroup(uint32_t index, BindGroup* bindGroup, std::vector<uint32_t> dynamicOffset)
{
}

void VulkanRenderBundleEncoder::setVertexBuffer(uint32_t slot, Buffer* buffer)
{
}

void VulkanRenderBundleEncoder::setIndexBuffer(Buffer* buffer, IndexFormat format)
{
}

void VulkanRenderBundleEncoder::draw(uint32_t vertexCount,
                                     uint32_t instanceCount,
                                     uint32_t firstVertex,
                                     uint32_t firstInstance)
{
}

void VulkanRenderBundleEncoder::drawIndexed(uint32_t indexCount,
                                            uint32_t instanceCount,
                                            uint32_t indexOffset,
                                            uint32_t vertexOffset,
                                            uint32_t firstInstance)
{
}

std::unique_ptr<RenderBundle> VulkanRenderBundleEncoder::finish(const RenderBundleDescriptor& descriptor)
{
    return VulkanRenderBundle::create(descriptor);
}

} // namespace jipu