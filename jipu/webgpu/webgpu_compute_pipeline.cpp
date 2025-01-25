#include "webgpu_compute_pipeline.h"

#include "webgpu_device.h"
#include "webgpu_pipeline_layout.h"
#include "webgpu_shader_module.h"
#include "webgpu_texture.h"

#include <cstring>

namespace jipu
{

WebGPUComputePipeline* WebGPUComputePipeline::create(WebGPUDevice* wgpuDevice, WGPUComputePipelineDescriptor const* descriptor)
{
    ComputePipelineDescriptor pipelineDescriptor{};

    // pipeline layout
    {
        pipelineDescriptor.layout = reinterpret_cast<WebGPUPipelineLayout*>(descriptor->layout)->getPipelineLayout();
    }

    // compute shader module
    {
        pipelineDescriptor.compute.entryPoint = std::string(descriptor->compute.entryPoint.data,
                                                            descriptor->compute.entryPoint.length != WGPU_STRLEN ? descriptor->compute.entryPoint.length : strlen(descriptor->compute.entryPoint.data));
        pipelineDescriptor.compute.shaderModule = reinterpret_cast<WebGPUShaderModule*>(descriptor->compute.module)->getShaderModule();
    }

    auto device = wgpuDevice->getDevice();
    auto renderPipeline = device->createComputePipeline(pipelineDescriptor);

    return new WebGPUComputePipeline(wgpuDevice, std::move(renderPipeline), descriptor);
}

WebGPUComputePipeline::WebGPUComputePipeline(WebGPUDevice* wgpuDevice, std::unique_ptr<ComputePipeline> pipeline, WGPUComputePipelineDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_pipeline(std::move(pipeline))
{
}

ComputePipeline* WebGPUComputePipeline::getComputePipeline() const
{
    return m_pipeline.get();
}

} // namespace jipu