#include "webgpu_command_encoder.h"

#include "webgpu_buffer.h"
#include "webgpu_command_buffer.h"
#include "webgpu_compute_pass_encoder.h"
#include "webgpu_device.h"
#include "webgpu_render_pass_encoder.h"
#include "webgpu_texture.h"
#include "webgpu_texture_view.h"

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

WebGPUComputePassEncoder* WebGPUCommandEncoder::beginComputePass(WGPUComputePassDescriptor const* descriptor)
{
    return WebGPUComputePassEncoder::create(this, descriptor);
}

void WebGPUCommandEncoder::copyBufferToBuffer(WGPUBuffer source, uint64_t sourceOffset, WGPUBuffer destination, uint64_t destinationOffset, uint64_t size)
{
    CopyBuffer srcBuffer{
        .buffer = reinterpret_cast<WebGPUBuffer*>(source)->getBuffer(),
        .offset = sourceOffset,
    };

    CopyBuffer dstBuffer{
        .buffer = reinterpret_cast<WebGPUBuffer*>(destination)->getBuffer(),
        .offset = destinationOffset,
    };

    m_commandEncoder->copyBufferToBuffer(srcBuffer, dstBuffer, size);
}

void WebGPUCommandEncoder::copyBufferToTexture(WGPUImageCopyBuffer const* source, WGPUImageCopyTexture const* destination, WGPUExtent3D const* copySize)
{
    CopyTextureBuffer buffer{
        .buffer = reinterpret_cast<WebGPUBuffer*>(source->buffer)->getBuffer(),
        .offset = source->layout.offset,
        .bytesPerRow = source->layout.bytesPerRow,
        .rowsPerTexture = source->layout.rowsPerImage,
    };

    auto wgpuTexture = reinterpret_cast<WebGPUTexture*>(destination->texture);
    CopyTexture texture{
        .texture = wgpuTexture->getTexture(),
        .aspect = WGPUToTextureAspectFlags(wgpuTexture, destination->aspect),
        .mipLevel = destination->mipLevel,
        // TODO: origin
    };

    Extent3D extent{
        .width = copySize->width,
        .height = copySize->height,
        .depth = copySize->depthOrArrayLayers,
    };

    m_commandEncoder->copyBufferToTexture(buffer, texture, extent);
}

void WebGPUCommandEncoder::copyTextureToBuffer(WGPUImageCopyTexture const* source, WGPUImageCopyBuffer const* destination, WGPUExtent3D const* copySize)
{
    auto wgpuSrcTexture = reinterpret_cast<WebGPUTexture*>(source->texture);
    CopyTexture texture{
        .texture = wgpuSrcTexture->getTexture(),
        .aspect = WGPUToTextureAspectFlags(wgpuSrcTexture, source->aspect),
    };

    CopyTextureBuffer buffer{
        .buffer = reinterpret_cast<WebGPUBuffer*>(destination->buffer)->getBuffer(),
        .offset = destination->layout.offset,
        .bytesPerRow = destination->layout.bytesPerRow,
        .rowsPerTexture = destination->layout.rowsPerImage,
    };

    Extent3D extent{
        .width = copySize->width,
        .height = copySize->height,
        .depth = copySize->depthOrArrayLayers,
    };

    m_commandEncoder->copyTextureToBuffer(texture, buffer, extent);
}

void WebGPUCommandEncoder::copyTextureToTexture(WGPUImageCopyTexture const* source, WGPUImageCopyTexture const* destination, WGPUExtent3D const* copySize)
{
    auto wgpuSrcTexture = reinterpret_cast<WebGPUTexture*>(source->texture);
    CopyTexture srcTexture{
        .texture = wgpuSrcTexture->getTexture(),
        .aspect = WGPUToTextureAspectFlags(wgpuSrcTexture, source->aspect),
    };

    auto wgpuDstTexture = reinterpret_cast<WebGPUTexture*>(destination->texture);
    CopyTexture dstTexture{
        .texture = reinterpret_cast<Texture*>(destination->texture),
        .aspect = WGPUToTextureAspectFlags(wgpuDstTexture, destination->aspect),
    };

    Extent3D extent{
        .width = copySize->width,
        .height = copySize->height,
        .depth = copySize->depthOrArrayLayers,
    };

    m_commandEncoder->copyTextureToTexture(srcTexture, dstTexture, extent);
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