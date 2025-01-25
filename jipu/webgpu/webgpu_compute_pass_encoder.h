
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/command_buffer.h"
#include "jipu/native/compute_pass_encoder.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUDevice;
class WebGPUCommandEncoder;
class WebGPUComputePassEncoder : public RefCounted
{

public:
    static WebGPUComputePassEncoder* create(WebGPUCommandEncoder* commandEncoder, WGPUComputePassDescriptor const* descriptor);

public:
    WebGPUComputePassEncoder() = delete;
    explicit WebGPUComputePassEncoder(WebGPUCommandEncoder* commandEncoder, std::unique_ptr<ComputePassEncoder> computePassEncoder, WGPUComputePassDescriptor const* descriptor);

public:
    virtual ~WebGPUComputePassEncoder() = default;

    WebGPUComputePassEncoder(const WebGPUComputePassEncoder&) = delete;
    WebGPUComputePassEncoder& operator=(const WebGPUComputePassEncoder&) = delete;

public: // WebGPU API
    void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);
    void setBindGroup(uint32_t groupIndex, WGPU_NULLABLE WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const* dynamicOffsets);
    void setPipeline(WGPUComputePipeline pipeline);
    void end();

public:
    ComputePassEncoder* getComputePassEncoder() const;

private:
    [[maybe_unused]] WebGPUCommandEncoder* m_wgpuCommandEncoder = nullptr;
    [[maybe_unused]] const WGPUComputePassDescriptor m_descriptor{};

private:
    std::unique_ptr<ComputePassEncoder> m_computePassEncoder = nullptr;
};

} // namespace jipu