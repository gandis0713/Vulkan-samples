
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/pipeline.h"
#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPURenderPipeline : public RefCounted
{

public:
    static WebGPURenderPipeline* create(WebGPUDevice* wgpuDevice, WGPURenderPipelineDescriptor const* descriptor);

public:
    WebGPURenderPipeline() = delete;
    explicit WebGPURenderPipeline(WebGPUDevice* device, std::unique_ptr<RenderPipeline> pipeline, WGPURenderPipelineDescriptor const* descriptor);

public:
    virtual ~WebGPURenderPipeline() = default;

    WebGPURenderPipeline(const WebGPURenderPipeline&) = delete;
    WebGPURenderPipeline& operator=(const WebGPURenderPipeline&) = delete;

public: // WebGPU API
public:
    RenderPipeline* getRenderPipeline() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPURenderPipelineDescriptor m_descriptor{};

private:
    std::unique_ptr<RenderPipeline> m_pipeline = nullptr;
};

// Convert from WebGPU to JIPU
WGPUVertexFormat ToWGPUVertexFormat(VertexFormat format);
WGPUVertexStepMode ToWGPUVertexStepMode(VertexMode mode);
WGPUPrimitiveTopology ToWGPUPrimitiveTopology(PrimitiveTopology topology);
WGPUCullMode ToWGPUCullMode(CullMode mode);
WGPUFrontFace ToWGPUFrontFace(FrontFace face);
WGPUBlendFactor ToWGPUBlendFactor(BlendFactor factor);
WGPUBlendOperation ToWGPUBlendOperation(BlendOperation operation);

// Convert from JIPU to WebGPU
VertexFormat ToVertexFormat(WGPUVertexFormat format);
VertexMode ToVertexMode(WGPUVertexStepMode mode);
PrimitiveTopology ToPrimitiveTopology(WGPUPrimitiveTopology topology);
CullMode ToCullMode(WGPUCullMode mode);
FrontFace ToFrontFace(WGPUFrontFace face);
BlendFactor ToBlendFactor(WGPUBlendFactor factor);
BlendOperation ToBlendOperation(WGPUBlendOperation operation);

} // namespace jipu