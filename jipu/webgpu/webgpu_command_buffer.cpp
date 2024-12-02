#include "webgpu_command_buffer.h"

namespace jipu
{

WebGPUCommandBuffer* WebGPUCommandBuffer::create(WebGPUCommandEncoder* wgpuCommandEncoder, WGPUCommandBufferDescriptor const* descriptor)
{
    return new WebGPUCommandBuffer(wgpuCommandEncoder, nullptr, descriptor);
}

WebGPUCommandBuffer::WebGPUCommandBuffer(WebGPUCommandEncoder* wgpuCommandEncoder, std::unique_ptr<CommandBuffer> commandBuffer, WGPUCommandBufferDescriptor const* descriptor)
    : m_wgpuCommandEncoder(wgpuCommandEncoder)
    , m_descriptor(*descriptor)
    , m_commandBuffer(std::move(commandBuffer))
{
}

CommandBuffer* WebGPUCommandBuffer::getCommandBuffer() const
{
    return m_commandBuffer.get();
}

} // namespace jipu