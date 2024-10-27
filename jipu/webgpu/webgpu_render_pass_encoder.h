
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/render_pass_encoder.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUCommandEncoder;
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
    void setPipeline(WGPURenderPipeline pipeline);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void end();

public:
    RenderPassEncoder* getRenderPassEncoder() const;

private:
    [[maybe_unused]] WebGPUCommandEncoder* m_wgpuCommandEncoder = nullptr;
    [[maybe_unused]] const WGPURenderPassDescriptor m_descriptor{};

private:
    std::unique_ptr<RenderPassEncoder> m_renderPassEncoder = nullptr;
};

// Convert from WebGPU to JIPU
WGPUColor ToWGPUColor(Color color);
WGPULoadOp ToWGPULoadOp(LoadOp loadOp);
WGPUStoreOp ToWGPUStoreOp(StoreOp storeOp);

// Convert from JIPU to WebGPU
Color ToColor(WGPUColor color);
LoadOp ToLoadOp(WGPULoadOp loadOp);
StoreOp ToStoreOp(WGPUStoreOp storeOp);

} // namespace jipu