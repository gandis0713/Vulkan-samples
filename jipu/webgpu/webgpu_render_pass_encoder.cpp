#include "webgpu_render_pass_encoder.h"

#include "webgpu_buffer.h"
#include "webgpu_command_encoder.h"
#include "webgpu_render_pipeline.h"
#include "webgpu_texture_view.h"

namespace jipu
{

WebGPURenderPassEncoder* WebGPURenderPassEncoder::create(WebGPUCommandEncoder* wgpuCommandEncoder, WGPURenderPassDescriptor const* descriptor)
{
    auto commandEncoder = wgpuCommandEncoder->getCommandEncoder();

    RenderPassEncoderDescriptor renderPassEncoderDescriptor{};

    // RenderPassEncoderDescriptor: colorAttachments
    {
        if (descriptor->colorAttachmentCount <= 0)
        {
            throw std::runtime_error("colorAttachmentCount must be greater than 0");
        }

        for (auto i = 0; i < descriptor->colorAttachmentCount; i++)
        {
            auto wgpuColorAttachment = descriptor->colorAttachments[i];

            ColorAttachment colorAttachment;
            colorAttachment.clearValue = WGPUToColor(wgpuColorAttachment.clearValue);
            colorAttachment.renderView = reinterpret_cast<WebGPUTextureView*>(wgpuColorAttachment.view)->getTextureView();
            colorAttachment.loadOp = WGPUToLoadOp(wgpuColorAttachment.loadOp);
            colorAttachment.storeOp = WGPUToStoreOp(wgpuColorAttachment.storeOp);

            auto resolveTarget = reinterpret_cast<WebGPUTextureView*>(wgpuColorAttachment.resolveTarget);
            if (resolveTarget)
            {
                colorAttachment.resolveView = resolveTarget->getTextureView();
            }

            renderPassEncoderDescriptor.colorAttachments.push_back(colorAttachment);
        }
    }

    // RenderPassEncoderDescriptor: depthStencilAttachment
    {
        if (descriptor->depthStencilAttachment)
        {
            const auto wgpuDepthStencilAttachment = descriptor->depthStencilAttachment;

            DepthStencilAttachment depthStencilAttachment;
            depthStencilAttachment.clearValue.depth = wgpuDepthStencilAttachment->depthClearValue;
            depthStencilAttachment.clearValue.stencil = wgpuDepthStencilAttachment->stencilClearValue;
            depthStencilAttachment.textureView = reinterpret_cast<WebGPUTextureView*>(wgpuDepthStencilAttachment->view)->getTextureView();
            depthStencilAttachment.depthLoadOp = WGPUToLoadOp(wgpuDepthStencilAttachment->depthLoadOp);
            depthStencilAttachment.depthStoreOp = WGPUToStoreOp(wgpuDepthStencilAttachment->depthStoreOp);
            depthStencilAttachment.stencilLoadOp = WGPUToLoadOp(wgpuDepthStencilAttachment->stencilLoadOp);
            depthStencilAttachment.stencilStoreOp = WGPUToStoreOp(wgpuDepthStencilAttachment->stencilStoreOp);

            renderPassEncoderDescriptor.depthStencilAttachment = depthStencilAttachment;
        }
    }

    // TODO: occlusionQuerySet
    // TODO: timestampWrites
    auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassEncoderDescriptor);

    return new WebGPURenderPassEncoder(wgpuCommandEncoder, std::move(renderPassEncoder), descriptor);
}

WebGPURenderPassEncoder::WebGPURenderPassEncoder(WebGPUCommandEncoder* wgpuCommandEncoder, std::unique_ptr<RenderPassEncoder> renderPassEncoder, WGPURenderPassDescriptor const* descriptor)
    : m_wgpuCommandEncoder(wgpuCommandEncoder)
    , m_descriptor(*descriptor)
    , m_renderPassEncoder(std::move(renderPassEncoder))
{
}

void WebGPURenderPassEncoder::setPipeline(WGPURenderPipeline wgpuPipeline)
{
    auto renderPipeline = reinterpret_cast<WebGPURenderPipeline*>(wgpuPipeline)->getRenderPipeline();
    m_renderPassEncoder->setPipeline(renderPipeline);

    // TODO: default viewport
    {
        auto textureView = reinterpret_cast<WebGPUTextureView*>(m_descriptor.colorAttachments[0].view)->getTextureView();
        auto width = textureView->getWidth();
        auto height = textureView->getHeight();
        m_renderPassEncoder->setViewport(0, 0, width, height, 0, 1);
    }

    // TODO: default scissor
    {
        auto textureView = reinterpret_cast<WebGPUTextureView*>(m_descriptor.colorAttachments[0].view)->getTextureView();
        auto width = textureView->getWidth();
        auto height = textureView->getHeight();
        m_renderPassEncoder->setScissor(0, 0, width, height);
    }
}

void WebGPURenderPassEncoder::setVertexBuffer(uint32_t slot, WebGPUBuffer* buffer, uint64_t offset, uint64_t size)
{
    m_renderPassEncoder->setVertexBuffer(slot, *buffer->getBuffer()); // TODO: offset, size
}

void WebGPURenderPassEncoder::setIndexBuffer(WebGPUBuffer* buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size)
{
    m_renderPassEncoder->setIndexBuffer(*buffer->getBuffer(), WGPUToIndexFormat(format)); // TODO: offset, size
}

void WebGPURenderPassEncoder::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    m_renderPassEncoder->setViewport(x, y, width, height, minDepth, maxDepth);
}

void WebGPURenderPassEncoder::setScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    m_renderPassEncoder->setScissor(x, y, width, height);
}

void WebGPURenderPassEncoder::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    m_renderPassEncoder->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void WebGPURenderPassEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    m_renderPassEncoder->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void WebGPURenderPassEncoder::end()
{
    m_renderPassEncoder->end();
}

RenderPassEncoder* WebGPURenderPassEncoder::getRenderPassEncoder() const
{
    return m_renderPassEncoder.get();
}

// Convert from JIPU to WebGPU
WGPUColor ToWGPUColor(Color color)
{
    WGPUColor wgpuColor{};
    wgpuColor.r = color.r;
    wgpuColor.g = color.g;
    wgpuColor.b = color.b;
    wgpuColor.a = color.a;

    return wgpuColor;
}

WGPULoadOp ToWGPULoadOp(LoadOp loadOp)
{
    switch (loadOp)
    {
    case LoadOp::kDontCare:
        return WGPULoadOp_Undefined;
    case LoadOp::kLoad:
        return WGPULoadOp_Load;
    case LoadOp::kClear:
        return WGPULoadOp_Clear;
    default:
        return WGPULoadOp_Undefined;
    }
}

WGPUStoreOp ToWGPUStoreOp(StoreOp storeOp)
{
    switch (storeOp)
    {
    case StoreOp::kDontCare:
        return WGPUStoreOp_Undefined;
    case StoreOp::kStore:
        return WGPUStoreOp_Store;
    default:
        return WGPUStoreOp_Undefined;
    }
}

WGPUIndexFormat ToWGPUIndexFormat(IndexFormat format)
{
    switch (format)
    {
    case IndexFormat::kUint16:
        return WGPUIndexFormat_Uint16;
    case IndexFormat::kUint32:
        return WGPUIndexFormat_Uint32;
    default:
        return WGPUIndexFormat_Undefined;
    }
}

// Convert from WebGPU to JIPU
Color WGPUToColor(WGPUColor color)
{
    Color jipuColor{};
    jipuColor.r = color.r;
    jipuColor.g = color.g;
    jipuColor.b = color.b;
    jipuColor.a = color.a;

    return jipuColor;
}

LoadOp WGPUToLoadOp(WGPULoadOp loadOp)
{
    switch (loadOp)
    {
    case WGPULoadOp_Undefined:
        return LoadOp::kDontCare;
    case WGPULoadOp_Load:
        return LoadOp::kLoad;
    case WGPULoadOp_Clear:
        return LoadOp::kClear;
    default:
        return LoadOp::kDontCare;
    }
}

StoreOp WGPUToStoreOp(WGPUStoreOp storeOp)
{
    switch (storeOp)
    {
    case WGPUStoreOp_Undefined:
        return StoreOp::kDontCare;
    case WGPUStoreOp_Store:
        return StoreOp::kStore;
    default:
        return StoreOp::kDontCare;
    }
}

IndexFormat WGPUToIndexFormat(WGPUIndexFormat format)
{
    switch (format)
    {
    case WGPUIndexFormat_Uint16:
        return IndexFormat::kUint16;
    case WGPUIndexFormat_Uint32:
        return IndexFormat::kUint32;
    default:
        return IndexFormat::kUndefined;
    }
}

} // namespace jipu