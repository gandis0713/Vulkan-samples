
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/render_bundle_encoder.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUBuffer;
class WebGPUBindGroup;
class WebGPUDevice;
class WebGPURenderPipeline;
class WebGPURenderBundle;
class WebGPURenderBundleEncoder : public RefCounted
{

public:
    static WebGPURenderBundleEncoder* create(WebGPUDevice* wgpuDevice, WGPURenderBundleEncoderDescriptor const* descriptor);

public:
    WebGPURenderBundleEncoder() = delete;
    explicit WebGPURenderBundleEncoder(WebGPUDevice* wgpuDevice, std::unique_ptr<RenderBundleEncoder> RenderBundleEncoder, WGPURenderBundleEncoderDescriptor const* descriptor);

public:
    virtual ~WebGPURenderBundleEncoder() = default;

    WebGPURenderBundleEncoder(const WebGPURenderBundleEncoder&) = delete;
    WebGPURenderBundleEncoder& operator=(const WebGPURenderBundleEncoder&) = delete;

public: // WebGPU API
    void setPipeline(WebGPURenderPipeline* pipeline);
    void setVertexBuffer(uint32_t slot, WebGPUBuffer* buffer, uint64_t offset, uint64_t size);
    void setIndexBuffer(WebGPUBuffer* buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size);
    void setBindGroup(uint32_t groupIndex, WGPU_NULLABLE WebGPUBindGroup* group, size_t dynamicOffsetCount, uint32_t const* dynamicOffsets);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
    WebGPURenderBundle* finish(WGPURenderBundleDescriptor const* descriptor);

public:
    RenderBundleEncoder* getRenderBundleEncoder() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPURenderBundleEncoderDescriptor m_descriptor{};

private:
    std::unique_ptr<RenderBundleEncoder> m_renderBundleEncoder = nullptr;
};

} // namespace jipu