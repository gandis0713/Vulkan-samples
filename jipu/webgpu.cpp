#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

extern WGPUProc procGetProcAddress(WGPUStringView procName);
extern WGPUInstance procCreateInstance(WGPUInstanceDescriptor const* wgpuDescriptor);
extern void procInstanceRequestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const* options, WGPURequestAdapterCallback callback, void* userdata);
extern WGPUSurface procInstanceCreateSurface(WGPUInstance instance, WGPUSurfaceDescriptor const* descriptor);
extern void procAdapterRequestDevice(WGPUAdapter adapter, WGPU_NULLABLE WGPUDeviceDescriptor const* descriptor, WGPURequestDeviceCallback callback, void* userdata);
extern WGPUQueue procDeviceGetQueue(WGPUDevice device);
extern WGPUStatus procSurfaceGetCapabilities(WGPUSurface surface, WGPUAdapter adapter, WGPUSurfaceCapabilities* capabilities);
extern void procSurfaceConfigure(WGPUSurface surface, WGPUSurfaceConfiguration const* config);
extern WGPUBindGroup procDeviceCreateBindGroup(WGPUDevice device, WGPUBindGroupDescriptor const* descriptor);
extern WGPUBindGroupLayout procDeviceCreateBindGroupLayout(WGPUDevice device, WGPUBindGroupLayoutDescriptor const* descriptor);
extern WGPUPipelineLayout procDeviceCreatePipelineLayout(WGPUDevice device, WGPUPipelineLayoutDescriptor const* descriptor);
extern WGPURenderPipeline procDeviceCreateRenderPipeline(WGPUDevice device, WGPURenderPipelineDescriptor const* descriptor);
extern WGPUShaderModule procDeviceCreateShaderModule(WGPUDevice device, WGPUShaderModuleDescriptor const* descriptor);
extern void procSurfaceGetCurrentTexture(WGPUSurface surface, WGPUSurfaceTexture* surfaceTexture);
extern WGPUTextureView procTextureCreateView(WGPUTexture texture, WGPU_NULLABLE WGPUTextureViewDescriptor const* descriptor);
extern WGPUCommandEncoder procDeviceCreateCommandEncoder(WGPUDevice device, WGPU_NULLABLE WGPUCommandEncoderDescriptor const* descriptor);
extern WGPURenderPassEncoder procCommandEncoderBeginRenderPass(WGPUCommandEncoder commandEncoder, WGPURenderPassDescriptor const* descriptor);
extern void procRenderPassEncoderSetPipeline(WGPURenderPassEncoder renderPassEncoder, WGPURenderPipeline pipeline);
extern void procRenderPassEncoderDraw(WGPURenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
extern void procRenderPassEncoderEnd(WGPURenderPassEncoder renderPassEncoder);
extern void procRenderPassEncoderRelease(WGPURenderPassEncoder renderPassEncoder);
extern WGPUCommandBuffer procCommandEncoderFinish(WGPUCommandEncoder commandEncoder, WGPU_NULLABLE WGPUCommandBufferDescriptor const* descriptor);
extern void procQueueSubmit(WGPUQueue queue, size_t commandCount, WGPUCommandBuffer const* commands);
extern void procSurfacePresent(WGPUSurface surface);
extern void procCommandBufferRelease(WGPUCommandBuffer commandBuffer);
extern void procCommandEncoderRelease(WGPUCommandEncoder commandEncoder);
extern void procTextureViewRelease(WGPUTextureView textureView);
extern void procTextureRelease(WGPUTexture texture);
extern void procRenderPipelineRelease(WGPURenderPipeline renderPipeline);
extern void procPipelineLayoutRelease(WGPUPipelineLayout pipelineLayout);
extern void procShaderModuleRelease(WGPUShaderModule shaderModule);
extern void procQueueRelease(WGPUQueue queue);
extern void procDeviceDestroy(WGPUDevice device);
extern void procDeviceRelease(WGPUDevice device);
extern void procAdapterRelease(WGPUAdapter adapter);
extern void procSurfaceRelease(WGPUSurface surface);
extern void procInstanceRelease(WGPUInstance instance);
extern WGPUTexture procDeviceCreateTexture(WGPUDevice device, WGPUTextureDescriptor const* descriptor);
extern WGPUBuffer procDeviceCreateBuffer(WGPUDevice device, WGPUBufferDescriptor const* descriptor);
extern void* procBufferGetMappedRange(WGPUBuffer buffer, size_t offset, size_t size);
extern void procBufferUnmap(WGPUBuffer buffer);
extern void procRenderPassEncoderSetVertexBuffer(WGPURenderPassEncoder renderPassEncoder, uint32_t slot, WGPU_NULLABLE WGPUBuffer buffer, uint64_t offset, uint64_t size);
extern void procRenderPassEncoderSetIndexBuffer(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size);
extern void procRenderPassEncoderDrawIndexed(WGPURenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
extern void procBufferRelease(WGPUBuffer buffer);
extern void procRenderPassEncoderSetViewport(WGPURenderPassEncoder renderPassEncoder, float x, float y, float width, float height, float minDepth, float maxDepth);
extern void procRenderPassEncoderSetScissorRect(WGPURenderPassEncoder renderPassEncoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern void procQueueWriteBuffer(WGPUQueue queue, WGPUBuffer buffer, uint64_t bufferOffset, void const* data, size_t size);
extern void procRenderPassEncoderSetBindGroup(WGPURenderPassEncoder renderPassEncoder, uint32_t groupIndex, WGPU_NULLABLE WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const* dynamicOffsets);

} // namespace jipu

extern "C"
{
    using namespace jipu;

    WGPUProc wgpuGetProcAddress(WGPUStringView procName) WGPU_FUNCTION_ATTRIBUTE
    {
        return procGetProcAddress(procName);
    }

    WGPU_EXPORT WGPUInstance wgpuCreateInstance(WGPU_NULLABLE WGPUInstanceDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procCreateInstance(descriptor);
    }

    WGPU_EXPORT void wgpuInstanceRequestAdapter(WGPUInstance instance, WGPU_NULLABLE WGPURequestAdapterOptions const* options, WGPURequestAdapterCallback callback, WGPU_NULLABLE void* userdata) WGPU_FUNCTION_ATTRIBUTE
    {
        return procInstanceRequestAdapter(instance, options, callback, userdata);
    }

    WGPU_EXPORT WGPUSurface wgpuInstanceCreateSurface(WGPUInstance instance, WGPUSurfaceDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procInstanceCreateSurface(instance, descriptor);
    }

    WGPU_EXPORT void wgpuAdapterRequestDevice(WGPUAdapter adapter, WGPU_NULLABLE WGPUDeviceDescriptor const* descriptor, WGPURequestDeviceCallback callback, void* userdata) WGPU_FUNCTION_ATTRIBUTE
    {
        return procAdapterRequestDevice(adapter, descriptor, callback, userdata);
    }

    WGPU_EXPORT WGPUQueue wgpuDeviceGetQueue(WGPUDevice device) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceGetQueue(device);
    }

    WGPU_EXPORT WGPUStatus wgpuSurfaceGetCapabilities(WGPUSurface surface, WGPUAdapter adapter, WGPUSurfaceCapabilities* capabilities) WGPU_FUNCTION_ATTRIBUTE
    {
        return procSurfaceGetCapabilities(surface, adapter, capabilities);
    }

    WGPU_EXPORT void wgpuSurfaceConfigure(WGPUSurface surface, WGPUSurfaceConfiguration const* config) WGPU_FUNCTION_ATTRIBUTE
    {
        return procSurfaceConfigure(surface, config);
    }

    WGPU_EXPORT WGPUBindGroup wgpuDeviceCreateBindGroup(WGPUDevice device, WGPUBindGroupDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreateBindGroup(device, descriptor);
    }

    WGPU_EXPORT WGPUBindGroupLayout wgpuDeviceCreateBindGroupLayout(WGPUDevice device, WGPUBindGroupLayoutDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreateBindGroupLayout(device, descriptor);
    }

    WGPU_EXPORT WGPUPipelineLayout wgpuDeviceCreatePipelineLayout(WGPUDevice device, WGPUPipelineLayoutDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreatePipelineLayout(device, descriptor);
    }

    WGPU_EXPORT WGPURenderPipeline wgpuDeviceCreateRenderPipeline(WGPUDevice device, WGPURenderPipelineDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreateRenderPipeline(device, descriptor);
    }

    WGPU_EXPORT WGPUShaderModule wgpuDeviceCreateShaderModule(WGPUDevice device, WGPUShaderModuleDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreateShaderModule(device, descriptor);
    }

    WGPU_EXPORT void wgpuSurfaceGetCurrentTexture(WGPUSurface surface, WGPUSurfaceTexture* surfaceTexture) WGPU_FUNCTION_ATTRIBUTE
    {
        return procSurfaceGetCurrentTexture(surface, surfaceTexture);
    }

    WGPU_EXPORT WGPUTextureView wgpuTextureCreateView(WGPUTexture texture, WGPU_NULLABLE WGPUTextureViewDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procTextureCreateView(texture, descriptor);
    }

    WGPU_EXPORT WGPUCommandEncoder wgpuDeviceCreateCommandEncoder(WGPUDevice device, WGPU_NULLABLE WGPUCommandEncoderDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreateCommandEncoder(device, descriptor);
    }

    WGPU_EXPORT WGPURenderPassEncoder wgpuCommandEncoderBeginRenderPass(WGPUCommandEncoder commandEncoder, WGPURenderPassDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procCommandEncoderBeginRenderPass(commandEncoder, descriptor);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderSetPipeline(WGPURenderPassEncoder renderPassEncoder, WGPURenderPipeline pipeline) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderSetPipeline(renderPassEncoder, pipeline);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderDraw(WGPURenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderDraw(renderPassEncoder, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderEnd(WGPURenderPassEncoder renderPassEncoder) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderEnd(renderPassEncoder);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderRelease(WGPURenderPassEncoder renderPassEncoder) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderRelease(renderPassEncoder);
    }

    WGPU_EXPORT WGPUCommandBuffer wgpuCommandEncoderFinish(WGPUCommandEncoder commandEncoder, WGPU_NULLABLE WGPUCommandBufferDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procCommandEncoderFinish(commandEncoder, descriptor);
    }

    WGPU_EXPORT void wgpuQueueSubmit(WGPUQueue queue, size_t commandCount, WGPUCommandBuffer const* commands) WGPU_FUNCTION_ATTRIBUTE
    {
        return procQueueSubmit(queue, commandCount, commands);
    }

    WGPU_EXPORT void wgpuSurfacePresent(WGPUSurface surface) WGPU_FUNCTION_ATTRIBUTE
    {
        return procSurfacePresent(surface);
    }

    WGPU_EXPORT void wgpuCommandBufferRelease(WGPUCommandBuffer commandBuffer) WGPU_FUNCTION_ATTRIBUTE
    {
        return procCommandBufferRelease(commandBuffer);
    }

    WGPU_EXPORT void wgpuCommandEncoderRelease(WGPUCommandEncoder commandEncoder) WGPU_FUNCTION_ATTRIBUTE
    {
        return procCommandEncoderRelease(commandEncoder);
    }

    WGPU_EXPORT void wgpuTextureViewRelease(WGPUTextureView textureView) WGPU_FUNCTION_ATTRIBUTE
    {
        return procTextureViewRelease(textureView);
    }

    WGPU_EXPORT void wgpuTextureRelease(WGPUTexture texture) WGPU_FUNCTION_ATTRIBUTE
    {
        return procTextureRelease(texture);
    }

    WGPU_EXPORT void wgpuRenderPipelineRelease(WGPURenderPipeline renderPipeline) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPipelineRelease(renderPipeline);
    }

    WGPU_EXPORT void wgpuPipelineLayoutRelease(WGPUPipelineLayout pipelineLayout) WGPU_FUNCTION_ATTRIBUTE
    {
        return procPipelineLayoutRelease(pipelineLayout);
    }

    WGPU_EXPORT void wgpuShaderModuleRelease(WGPUShaderModule shaderModule) WGPU_FUNCTION_ATTRIBUTE
    {
        return procShaderModuleRelease(shaderModule);
    }

    WGPU_EXPORT void wgpuQueueRelease(WGPUQueue queue) WGPU_FUNCTION_ATTRIBUTE
    {
        return procQueueRelease(queue);
    }

    WGPU_EXPORT void wgpuDeviceDestroy(WGPUDevice device) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceDestroy(device);
    }

    WGPU_EXPORT void wgpuDeviceRelease(WGPUDevice device) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceRelease(device);
    }

    WGPU_EXPORT void wgpuAdapterRelease(WGPUAdapter adapter) WGPU_FUNCTION_ATTRIBUTE
    {
        return procAdapterRelease(adapter);
    }

    WGPU_EXPORT void wgpuSurfaceRelease(WGPUSurface surface) WGPU_FUNCTION_ATTRIBUTE
    {
        return procSurfaceRelease(surface);
    }

    WGPU_EXPORT void wgpuInstanceRelease(WGPUInstance instance) WGPU_FUNCTION_ATTRIBUTE
    {
        return procInstanceRelease(instance);
    }

    WGPU_EXPORT WGPUTexture wgpuDeviceCreateTexture(WGPUDevice device, WGPUTextureDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreateTexture(device, descriptor);
    }

    WGPU_EXPORT WGPUBuffer wgpuDeviceCreateBuffer(WGPUDevice device, WGPUBufferDescriptor const* descriptor) WGPU_FUNCTION_ATTRIBUTE
    {
        return procDeviceCreateBuffer(device, descriptor);
    }

    WGPU_EXPORT void* wgpuBufferGetMappedRange(WGPUBuffer buffer, size_t offset, size_t size) WGPU_FUNCTION_ATTRIBUTE
    {
        return procBufferGetMappedRange(buffer, offset, size);
    }

    WGPU_EXPORT void wgpuBufferUnmap(WGPUBuffer buffer) WGPU_FUNCTION_ATTRIBUTE
    {
        return procBufferUnmap(buffer);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderSetVertexBuffer(WGPURenderPassEncoder renderPassEncoder, uint32_t slot, WGPU_NULLABLE WGPUBuffer buffer, uint64_t offset, uint64_t size) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderSetVertexBuffer(renderPassEncoder, slot, buffer, offset, size);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderSetIndexBuffer(WGPURenderPassEncoder renderPassEncoder, WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderSetIndexBuffer(renderPassEncoder, buffer, format, offset, size);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderDrawIndexed(WGPURenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderDrawIndexed(renderPassEncoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

    WGPU_EXPORT void wgpuBufferRelease(WGPUBuffer buffer) WGPU_FUNCTION_ATTRIBUTE
    {
        return procBufferRelease(buffer);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderSetViewport(WGPURenderPassEncoder renderPassEncoder, float x, float y, float width, float height, float minDepth, float maxDepth) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderSetViewport(renderPassEncoder, x, y, width, height, minDepth, maxDepth);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderSetScissorRect(WGPURenderPassEncoder renderPassEncoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderSetScissorRect(renderPassEncoder, x, y, width, height);
    }

    WGPU_EXPORT void wgpuQueueWriteBuffer(WGPUQueue queue, WGPUBuffer buffer, uint64_t bufferOffset, void const* data, size_t size) WGPU_FUNCTION_ATTRIBUTE
    {
        return procQueueWriteBuffer(queue, buffer, bufferOffset, data, size);
    }

    WGPU_EXPORT void wgpuRenderPassEncoderSetBindGroup(WGPURenderPassEncoder renderPassEncoder, uint32_t groupIndex, WGPU_NULLABLE WGPUBindGroup group, size_t dynamicOffsetCount, uint32_t const* dynamicOffsets) WGPU_FUNCTION_ATTRIBUTE
    {
        return procRenderPassEncoderSetBindGroup(renderPassEncoder, groupIndex, group, dynamicOffsetCount, dynamicOffsets);
    }
}