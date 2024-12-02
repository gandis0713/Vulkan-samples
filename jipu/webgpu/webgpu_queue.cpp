#include "webgpu_queue.h"

#include "webgpu_buffer.h"
#include "webgpu_command_buffer.h"
#include "webgpu_device.h"

#include "jipu/native/buffer.h"
#include "jipu/native/queue.h"

namespace jipu
{

WebGPUQueue* WebGPUQueue::create(WebGPUDevice* wgpuDevice, WGPUQueueDescriptor const* descriptor)
{
    auto device = wgpuDevice->getDevice();

    auto queue = device->createQueue(QueueDescriptor{});
    return new WebGPUQueue(wgpuDevice, std::move(queue), descriptor);
}

WebGPUQueue::WebGPUQueue(WebGPUDevice* wgpuDevice, std::unique_ptr<Queue> queue, WGPUQueueDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_queue(std::move(queue))
{
}

void WebGPUQueue::submit(size_t commandCount, WGPUCommandBuffer const* commands)
{
    std::vector<CommandBuffer*> commandBuffers{};

    for (auto i = 0; i < commandCount; ++i)
    {
        auto commandBuffer = reinterpret_cast<WebGPUCommandBuffer*>(commands[i])->getCommandBuffer();
        commandBuffers.push_back(commandBuffer);
    }

    m_queue->submit(commandBuffers);
}

void WebGPUQueue::writeBuffer(WebGPUBuffer* buffer, uint64_t bufferOffset, void const* data, size_t size)
{
    BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = size;
    bufferDescriptor.usage = BufferUsageFlagBits::kCopySrc;

    auto device = m_wgpuDevice->getDevice();
    auto srcBuffer = device->createBuffer(bufferDescriptor);
    void* ptr = srcBuffer->map();
    memcpy(ptr, data, bufferDescriptor.size);
    srcBuffer->unmap();

    CopyBuffer srcCopyBuffer{
        .buffer = srcBuffer.get(),
        .offset = bufferOffset,
    };

    CopyBuffer dstCopyBuffer{
        .buffer = buffer->getBuffer(),
        .offset = bufferOffset,
    };

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = device->createCommandEncoder(commandEncoderDescriptor);

    commandEncoder->copyBufferToBuffer(srcCopyBuffer, dstCopyBuffer, size);
    auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
    m_queue->submit({ commandBuffer.get() });
}

Queue* WebGPUQueue::getQueue() const
{
    return m_queue.get();
}

} // namespace jipu