#include "webgpu_header.h"

namespace jipu
{

extern WGPUProc procGetProcAddress(WGPUDevice device, char const* procName);
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

} // namespace jipu

extern "C"
{
    using namespace jipu;

    WGPU_EXPORT WGPUProc wgpuGetProcAddress(WGPUDevice device, char const* procName) WGPU_FUNCTION_ATTRIBUTE
    {
        return procGetProcAddress(device, procName);
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
}