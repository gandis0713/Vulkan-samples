#include "webgpu_compute_pass_encoder.h"

#include "webgpu_bind_group.h"
#include "webgpu_command_encoder.h"
#include "webgpu_compute_pipeline.h"
#include "webgpu_device.h"

namespace jipu
{

WebGPUComputePassEncoder* WebGPUComputePassEncoder::create(WebGPUCommandEncoder* commandEncoder, WGPUComputePassDescriptor const* descriptor)
{
    ComputePassEncoderDescriptor computePassEncoderDescriptor{};
    auto computePassEncoder = commandEncoder->getCommandEncoder()->beginComputePass(computePassEncoderDescriptor);

    return new WebGPUComputePassEncoder(commandEncoder, std::move(computePassEncoder), descriptor);
}

WebGPUComputePassEncoder::WebGPUComputePassEncoder(WebGPUCommandEncoder* commandEncoder, std::unique_ptr<ComputePassEncoder> computePassEncoder, WGPUComputePassDescriptor const* descriptor)
    : m_wgpuCommandEncoder(commandEncoder)
    , m_descriptor(*descriptor)
    , m_computePassEncoder(std::move(computePassEncoder))
{
}

void WebGPUComputePassEncoder::dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    m_computePassEncoder->dispatch(workgroupCountX, workgroupCountY, workgroupCountZ);
}

void WebGPUComputePassEncoder::setBindGroup(uint32_t groupIndex, WGPU_NULLABLE WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const* dynamicOffsets)
{
    auto bindGroup = reinterpret_cast<WebGPUBindGroup*>(group);

    std::vector<uint32_t> dynamicOffset{};
    for (size_t i = 0; i < dynamicOffsetCount; i++)
    {
        dynamicOffset.push_back(dynamicOffsets[i]);
    }

    m_computePassEncoder->setBindGroup(groupIndex, bindGroup->getBindGroup(), dynamicOffset);
}

void WebGPUComputePassEncoder::setPipeline(WGPUComputePipeline pipeline)
{
    auto computePipeline = reinterpret_cast<WebGPUComputePipeline*>(pipeline);
    m_computePassEncoder->setPipeline(computePipeline->getComputePipeline());
}

void WebGPUComputePassEncoder::end()
{
    m_computePassEncoder->end();
}

} // namespace jipu