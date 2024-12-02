#include "webgpu_command_encoder.h"

#include "webgpu_command_buffer.h"
#include "webgpu_device.h"
#include "webgpu_render_pass_encoder.h"

namespace jipu
{

WebGPUCommandEncoder* WebGPUCommandEncoder::create(WebGPUDevice* wgpuDevice, WGPUCommandEncoderDescriptor const* descriptor)
{
    auto device = wgpuDevice->getDevice();

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = device->createCommandEncoder(commandEncoderDescriptor);

    return new WebGPUCommandEncoder(wgpuDevice, std::move(commandEncoder), descriptor);
}

WebGPUCommandEncoder::WebGPUCommandEncoder(WebGPUDevice* wgpuDevice, std::unique_ptr<CommandEncoder> commandEncoder, WGPUCommandEncoderDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_commandEncoder(std::move(commandEncoder))
{
}

WebGPURenderPassEncoder* WebGPUCommandEncoder::beginRenderPass(WGPURenderPassDescriptor const* descriptor)
{
    return WebGPURenderPassEncoder::create(this, descriptor);
}

WebGPUCommandBuffer* WebGPUCommandEncoder::finish(WGPUCommandBufferDescriptor const* descriptor)
{
    [[maybe_unused]] auto commandBuffer = m_commandEncoder->finish(CommandBufferDescriptor{});
    // TODO: create command buffer by descriptor here
    return new WebGPUCommandBuffer(this, std::move(commandBuffer), descriptor);
}

CommandEncoder* WebGPUCommandEncoder::getCommandEncoder() const
{
    return m_commandEncoder.get();
}

} // namespace jipu