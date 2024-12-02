
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/command_buffer.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUCommandEncoder;
class WebGPUCommandBuffer : public RefCounted
{

public:
    static WebGPUCommandBuffer* create(WebGPUCommandEncoder* wgpuCommandEncoder, WGPUCommandBufferDescriptor const* descriptor);

public:
    WebGPUCommandBuffer() = delete;
    explicit WebGPUCommandBuffer(WebGPUCommandEncoder* wgpuCommandEncoder, std::unique_ptr<CommandBuffer> commandBuffer, WGPUCommandBufferDescriptor const* descriptor);

public:
    virtual ~WebGPUCommandBuffer() = default;

    WebGPUCommandBuffer(const WebGPUCommandBuffer&) = delete;
    WebGPUCommandBuffer& operator=(const WebGPUCommandBuffer&) = delete;

public: // WebGPU API
public:
    CommandBuffer* getCommandBuffer() const;

private:
    [[maybe_unused]] WebGPUCommandEncoder* m_wgpuCommandEncoder = nullptr;
    [[maybe_unused]] const WGPUCommandBufferDescriptor m_descriptor{};

private:
    std::unique_ptr<CommandBuffer> m_commandBuffer = nullptr;
};

} // namespace jipu