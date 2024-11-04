
#pragma once

#include <memory>

#include "jipu/common/ref_counted.h"
#include "jipu/native/buffer.h"
#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUBuffer : public RefCounted
{

public:
    static WebGPUBuffer* create(WebGPUDevice* device, WGPUBufferDescriptor const* descriptor);

public:
    WebGPUBuffer() = delete;
    explicit WebGPUBuffer(WebGPUDevice* device, std::unique_ptr<Buffer> buffer, WGPUBufferDescriptor const* descriptor);

public:
    virtual ~WebGPUBuffer() = default;

    WebGPUBuffer(const WebGPUBuffer&) = delete;
    WebGPUBuffer& operator=(const WebGPUBuffer&) = delete;

public: // WebGPU API
public:
    Buffer* getBuffer() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUBufferDescriptor m_descriptor{};

private:
    std::unique_ptr<Buffer> m_buffer = nullptr;
};

// Convert from JIPU to WebGPU
WGPUBufferUsage ToWGPUBufferUsage(BufferUsageFlags usage);

// Convert from WebGPU to JIPU
BufferUsageFlags ToBufferUsageFlags(WGPUBufferUsage usage);

} // namespace jipu