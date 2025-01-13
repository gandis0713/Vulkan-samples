
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/render_pass_encoder.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUBuffer;
class WebGPUBindGroup;
class WebGPUCommandEncoder;
class WebGPURenderPipeline;
class WebGPURenderPassEncoder : public RefCounted
{

public:
    static WebGPURenderPassEncoder* create(WebGPUCommandEncoder* wgpuCommandEncoder, WGPURenderPassDescriptor const* descriptor);

public:
    WebGPURenderPassEncoder() = delete;
    explicit WebGPURenderPassEncoder(WebGPUCommandEncoder* wgpuCommandEncoder, std::unique_ptr<RenderPassEncoder> renderPassEncoder, WGPURenderPassDescriptor const* descriptor);

public:
    virtual ~WebGPURenderPassEncoder() = default;

    WebGPURenderPassEncoder(const WebGPURenderPassEncoder&) = delete;
    WebGPURenderPassEncoder& operator=(const WebGPURenderPassEncoder&) = delete;

public: // WebGPU API
    void setPipeline(WebGPURenderPipeline* pipeline);
    void setVertexBuffer(uint32_t slot, WebGPUBuffer* buffer, uint64_t offset, uint64_t size);
    void setIndexBuffer(WebGPUBuffer* buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size);
    void setBindGroup(uint32_t groupIndex, WGPU_NULLABLE WebGPUBindGroup* group, size_t dynamicOffsetCount, uint32_t const* dynamicOffsets);
    void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
    void setScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void setBlendConstant(WGPUColor const* color);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
    void executeBundles(size_t bundleCount, WGPURenderBundle const* bundles);
    void end();

public:
    RenderPassEncoder* getRenderPassEncoder() const;

private:
    [[maybe_unused]] WebGPUCommandEncoder* m_wgpuCommandEncoder = nullptr;
    [[maybe_unused]] const WGPURenderPassDescriptor m_descriptor{};

private:
    std::unique_ptr<RenderPassEncoder> m_renderPassEncoder = nullptr;
};

// Convert from JIPU to WebGPU
WGPUColor ToWGPUColor(Color color);
WGPULoadOp ToWGPULoadOp(LoadOp loadOp);
WGPUStoreOp ToWGPUStoreOp(StoreOp storeOp);
WGPUIndexFormat ToWGPUIndexFormat(IndexFormat format);

// Convert from WebGPU to JIPU
Color WGPUToColor(WGPUColor color);
LoadOp WGPUToLoadOp(WGPULoadOp loadOp);
StoreOp WGPUToStoreOp(WGPUStoreOp storeOp);
IndexFormat WGPUToIndexFormat(WGPUIndexFormat format);

} // namespace jipu