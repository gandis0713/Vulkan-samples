
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/pipeline.h"
#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUComputePipeline : public RefCounted
{

public:
    static WebGPUComputePipeline* create(WebGPUDevice* wgpuDevice, WGPUComputePipelineDescriptor const* descriptor);

public:
    WebGPUComputePipeline() = delete;
    explicit WebGPUComputePipeline(WebGPUDevice* device, std::unique_ptr<ComputePipeline> pipeline, WGPUComputePipelineDescriptor const* descriptor);

public:
    virtual ~WebGPUComputePipeline() = default;

    WebGPUComputePipeline(const WebGPUComputePipeline&) = delete;
    WebGPUComputePipeline& operator=(const WebGPUComputePipeline&) = delete;

public: // WebGPU API
public:
    ComputePipeline* getComputePipeline() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUComputePipelineDescriptor m_descriptor{};

private:
    std::unique_ptr<ComputePipeline> m_pipeline = nullptr;
};

} // namespace jipu