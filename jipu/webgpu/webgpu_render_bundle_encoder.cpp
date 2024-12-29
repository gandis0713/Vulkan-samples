#include "webgpu_render_bundle_encoder.h"

#include "webgpu_bind_group.h"
#include "webgpu_buffer.h"
#include "webgpu_device.h"
#include "webgpu_render_bundle.h"
#include "webgpu_render_pass_encoder.h"
#include "webgpu_render_pipeline.h"
#include "webgpu_texture_view.h"

namespace jipu
{

WebGPURenderBundleEncoder* WebGPURenderBundleEncoder::create(WebGPUDevice* wgpuDevice, WGPURenderPassDescriptor const* descriptor)
{
    // auto device = wgpuDevice->getDevice();

    // RenderBundleEncoderDescriptor renderBundleEncoderDescriptor{};

    // return new WebGPURenderBundleEncoder(wgpuDevice, std::move(renderBundleEncoder), descriptor);
    return nullptr;
}

WebGPURenderBundleEncoder::WebGPURenderBundleEncoder(WebGPUDevice* wgpuDevice, std::unique_ptr<RenderBundleEncoder> renderBundleEncoder, WGPURenderPassDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_renderBundleEncoder(std::move(renderBundleEncoder))
{
}

void WebGPURenderBundleEncoder::setPipeline(WebGPURenderPipeline* wgpuPipeline)
{
    auto renderPipeline = wgpuPipeline->getRenderPipeline();
    m_renderBundleEncoder->setPipeline(renderPipeline);
}

void WebGPURenderBundleEncoder::setVertexBuffer(uint32_t slot, WebGPUBuffer* buffer, uint64_t offset, uint64_t size)
{
    m_renderBundleEncoder->setVertexBuffer(slot, buffer->getBuffer()); // TODO: offset, size
}

void WebGPURenderBundleEncoder::setIndexBuffer(WebGPUBuffer* buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size)
{
    m_renderBundleEncoder->setIndexBuffer(buffer->getBuffer(), WGPUToIndexFormat(format)); // TODO: offset, size
}

void WebGPURenderBundleEncoder::setBindGroup(uint32_t groupIndex, WGPU_NULLABLE WebGPUBindGroup* group, size_t dynamicOffsetCount, uint32_t const* dynamicOffsets)
{
    auto bindGroup = group->getBindGroup();

    std::vector<uint32_t> dynamicOffsetsVec(dynamicOffsets, dynamicOffsets + dynamicOffsetCount);
    m_renderBundleEncoder->setBindGroup(groupIndex, bindGroup, dynamicOffsetsVec);
}

void WebGPURenderBundleEncoder::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    m_renderBundleEncoder->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void WebGPURenderBundleEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    m_renderBundleEncoder->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

WebGPURenderBundle* WebGPURenderBundleEncoder::finish(WGPURenderBundleDescriptor const* descriptor)
{
    RenderBundleDescriptor renderBundleDescriptor{};
    auto renderBundle = m_renderBundleEncoder->finish(renderBundleDescriptor);

    return WebGPURenderBundle::create(this, descriptor);
}

RenderBundleEncoder* WebGPURenderBundleEncoder::getRenderBundleEncoder() const
{
    return m_renderBundleEncoder.get();
}

} // namespace jipu