
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/command_buffer.h"
#include "jipu/native/command_encoder.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUDevice;
class WebGPURenderPassEncoder;
class WebGPUCommandBuffer;
class WebGPUCommandEncoder : public RefCounted
{

public:
    static WebGPUCommandEncoder* create(WebGPUDevice* wgpuDevice, WGPUCommandEncoderDescriptor const* descriptor);

public:
    WebGPUCommandEncoder() = delete;
    explicit WebGPUCommandEncoder(WebGPUDevice* wgpuDevice, std::unique_ptr<CommandEncoder> commandEncoder, WGPUCommandEncoderDescriptor const* descriptor);

public:
    virtual ~WebGPUCommandEncoder() = default;

    WebGPUCommandEncoder(const WebGPUCommandEncoder&) = delete;
    WebGPUCommandEncoder& operator=(const WebGPUCommandEncoder&) = delete;

public: // WebGPU API
    WebGPURenderPassEncoder* beginRenderPass(WGPURenderPassDescriptor const* descriptor);
    void copyBufferToBuffer(WGPUBuffer source, uint64_t sourceOffset, WGPUBuffer destination, uint64_t destinationOffset, uint64_t size);
    void copyBufferToTexture(WGPUImageCopyBuffer const* source, WGPUImageCopyTexture const* destination, WGPUExtent3D const* copySize);
    void copyTextureToBuffer(WGPUImageCopyTexture const* source, WGPUImageCopyBuffer const* destination, WGPUExtent3D const* copySize);
    void copyTextureToTexture(WGPUImageCopyTexture const* source, WGPUImageCopyTexture const* destination, WGPUExtent3D const* copySize);
    WebGPUCommandBuffer* finish(WGPUCommandBufferDescriptor const* descriptor);

public:
    CommandEncoder* getCommandEncoder() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUCommandEncoderDescriptor m_descriptor{};

private:
    std::unique_ptr<CommandEncoder> m_commandEncoder = nullptr;
};

} // namespace jipu