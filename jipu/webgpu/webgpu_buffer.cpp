#include "webgpu_buffer.h"

#include "webgpu_device.h"

namespace jipu
{

WebGPUBuffer* WebGPUBuffer::create(WebGPUDevice* device, WGPUBufferDescriptor const* descriptor)
{
    BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = descriptor->size;
    bufferDescriptor.usage = ToBufferUsageFlags(descriptor->usage);

    auto buffer = device->getDevice()->createBuffer(bufferDescriptor);

    return new WebGPUBuffer(device, std::move(buffer), descriptor);
}

WebGPUBuffer::WebGPUBuffer(WebGPUDevice* device, std::unique_ptr<Buffer> buffer, WGPUBufferDescriptor const* descriptor)
    : m_wgpuDevice(device)
    , m_descriptor(*descriptor)
    , m_buffer(std::move(buffer))
{
}

void* WebGPUBuffer::getMappedRange(size_t offset, size_t size)
{
    return m_buffer->map(); // TODO: offset, size
}

void WebGPUBuffer::unmap()
{
    m_buffer->unmap();
}

uint64_t WebGPUBuffer::getSize() const
{
    return m_buffer->getSize();
}

Buffer* WebGPUBuffer::getBuffer() const
{
    return m_buffer.get();
}

// Convert from JIPU to WebGPU
WGPUBufferUsage ToWGPUBufferUsage(BufferUsageFlags usage)
{
    WGPUBufferUsage wgpuUsage = WGPUBufferUsage_None;

    if (usage & BufferUsageFlagBits::kCopyDst)
    {
        wgpuUsage |= WGPUBufferUsage_CopyDst;
    }
    if (usage & BufferUsageFlagBits::kCopySrc)
    {
        wgpuUsage |= WGPUBufferUsage_CopySrc;
    }
    if (usage & BufferUsageFlagBits::kIndex)
    {
        wgpuUsage |= WGPUBufferUsage_Index;
    }
    if (usage & BufferUsageFlagBits::kVertex)
    {
        wgpuUsage |= WGPUBufferUsage_Vertex;
    }
    if (usage & BufferUsageFlagBits::kMapRead)
    {
        wgpuUsage |= WGPUBufferUsage_MapRead;
    }
    if (usage & BufferUsageFlagBits::kMapWrite)
    {
        wgpuUsage |= WGPUBufferUsage_MapWrite;
    }
    if (usage & BufferUsageFlagBits::kStorage)
    {
        wgpuUsage |= WGPUBufferUsage_Storage;
    }
    if (usage & BufferUsageFlagBits::kUniform)
    {
        wgpuUsage |= WGPUBufferUsage_Uniform;
    }

    return wgpuUsage;
}

// Convert from WebGPU to JIPU
BufferUsageFlags ToBufferUsageFlags(WGPUBufferUsage usage)
{
    BufferUsageFlags jipuUsage = BufferUsageFlagBits::kUndefined;

    if (usage & WGPUBufferUsage_CopyDst)
    {
        jipuUsage |= BufferUsageFlagBits::kCopyDst;
    }
    if (usage & WGPUBufferUsage_CopySrc)
    {
        jipuUsage |= BufferUsageFlagBits::kCopySrc;
    }
    if (usage & WGPUBufferUsage_Index)
    {
        jipuUsage |= BufferUsageFlagBits::kIndex;
    }
    if (usage & WGPUBufferUsage_Vertex)
    {
        jipuUsage |= BufferUsageFlagBits::kVertex;
    }
    if (usage & WGPUBufferUsage_MapRead)
    {
        jipuUsage |= BufferUsageFlagBits::kMapRead;
    }
    if (usage & WGPUBufferUsage_MapWrite)
    {
        jipuUsage |= BufferUsageFlagBits::kMapWrite;
    }
    if (usage & WGPUBufferUsage_Storage)
    {
        jipuUsage |= BufferUsageFlagBits::kStorage;
    }
    if (usage & WGPUBufferUsage_Uniform)
    {
        jipuUsage |= BufferUsageFlagBits::kUniform;
    }

    return jipuUsage;
}

} // namespace jipu