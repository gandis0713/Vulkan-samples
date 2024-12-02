#include "webgpu_pipeline_layout.h"

#include "webgpu_bind_group_layout.h"
#include "webgpu_device.h"

namespace jipu
{

WebGPUPipelineLayout* WebGPUPipelineLayout::create(WebGPUDevice* wgpuDevice, WGPUPipelineLayoutDescriptor const* descriptor)
{
    PipelineLayoutDescriptor layoutDescriptor{};

    for (auto i = 0; i < descriptor->bindGroupLayoutCount; ++i)
    {
        layoutDescriptor.layouts.push_back(reinterpret_cast<WebGPUBindGroupLayout*>(descriptor->bindGroupLayouts[i])->getBindGroupLayout());
    }

    auto device = wgpuDevice->getDevice();
    auto layout = device->createPipelineLayout(layoutDescriptor);

    return new WebGPUPipelineLayout(wgpuDevice, std::move(layout), descriptor);
}

WebGPUPipelineLayout::WebGPUPipelineLayout(WebGPUDevice* wgpuDevice, std::unique_ptr<PipelineLayout> layout, WGPUPipelineLayoutDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_layout(std::move(layout))
{
}

PipelineLayout* WebGPUPipelineLayout::getPipelineLayout() const
{
    return m_layout.get();
}

} // namespace jipu