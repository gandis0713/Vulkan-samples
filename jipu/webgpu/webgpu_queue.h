#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/queue.h"
#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUBuffer;
class WebGPUQueue : public RefCounted
{
public:
    static WebGPUQueue* create(WebGPUDevice* wgpuDevice, WGPUQueueDescriptor const* wgpuDescriptor);

public:
    WebGPUQueue() = delete;
    explicit WebGPUQueue(WebGPUDevice* wgpuDevice, std::unique_ptr<Queue> queue, WGPUQueueDescriptor const* wgpuDescriptor);

public:
    virtual ~WebGPUQueue() = default;

    WebGPUQueue(const WebGPUQueue&) = delete;
    WebGPUQueue& operator=(const WebGPUQueue&) = delete;

public: // WebGPU API
    void submit(size_t commandCount, WGPUCommandBuffer const* commands);
    void writeBuffer(WebGPUBuffer* buffer, uint64_t bufferOffset, void const* data, size_t size);

public:
    Queue* getQueue() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUQueueDescriptor m_descriptor{};

private:
    std::unique_ptr<Queue> m_queue = nullptr;
};

} // namespace jipu