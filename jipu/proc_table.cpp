#include "jipu/webgpu/webgpu_header.h"

#include "webgpu/webgpu_adapter.h"
#include "webgpu/webgpu_command_buffer.h"
#include "webgpu/webgpu_command_encoder.h"
#include "webgpu/webgpu_device.h"
#include "webgpu/webgpu_instance.h"
#include "webgpu/webgpu_pipeline_layout.h"
#include "webgpu/webgpu_queue.h"
#include "webgpu/webgpu_render_pass_encoder.h"
#include "webgpu/webgpu_render_pipeline.h"
#include "webgpu/webgpu_shader_module.h"
#include "webgpu/webgpu_surface.h"
#include "webgpu/webgpu_texture.h"
#include "webgpu/webgpu_texture_view.h"

#include <unordered_map>

namespace jipu
{

WGPUInstance procCreateInstance(WGPUInstanceDescriptor const* wgpuDescriptor)
{
    return reinterpret_cast<WGPUInstance>(WebGPUInstance::create(wgpuDescriptor));
}

void procInstanceRequestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const* options, WGPURequestAdapterCallback callback, void* userdata)
{
    WebGPUInstance* webgpuInstance = reinterpret_cast<WebGPUInstance*>(instance);
    return webgpuInstance->requestAdapter(options, callback, userdata);
}

WGPUSurface procInstanceCreateSurface(WGPUInstance instance, WGPUSurfaceDescriptor const* descriptor)
{
    WebGPUInstance* webgpuInstance = reinterpret_cast<WebGPUInstance*>(instance);
    return reinterpret_cast<WGPUSurface>(webgpuInstance->createSurface(descriptor));
}

void procAdapterRequestDevice(WGPUAdapter adapter, WGPU_NULLABLE WGPUDeviceDescriptor const* descriptor, WGPURequestDeviceCallback callback, void* userdata)
{
    WebGPUAdapter* webgpuAdapter = reinterpret_cast<WebGPUAdapter*>(adapter);
    return webgpuAdapter->requestDevice(descriptor, callback, userdata);
}

WGPUQueue procDeviceGetQueue(WGPUDevice device)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return reinterpret_cast<WGPUQueue>(webgpuDevice->getQueue());
}

WGPUStatus procSurfaceGetCapabilities(WGPUSurface surface, WGPUAdapter adapter, WGPUSurfaceCapabilities* capabilities)
{
    WebGPUSurface* webgpuSurface = reinterpret_cast<WebGPUSurface*>(surface);
    WebGPUAdapter* webgpuAdapter = reinterpret_cast<WebGPUAdapter*>(adapter);
    return webgpuSurface->getCapabilities(webgpuAdapter, capabilities);
}

void procSurfaceConfigure(WGPUSurface surface, WGPUSurfaceConfiguration const* config)
{
    WebGPUSurface* webgpuSurface = reinterpret_cast<WebGPUSurface*>(surface);
    return webgpuSurface->configure(config);
}

WGPUBindGroup procDeviceCreateBindGroup(WGPUDevice device, WGPUBindGroupDescriptor const* descriptor)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return reinterpret_cast<WGPUBindGroup>(webgpuDevice->createBindGroup(descriptor));
}

WGPUBindGroupLayout procDeviceCreateBindGroupLayout(WGPUDevice device, WGPUBindGroupLayoutDescriptor const* descriptor)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return reinterpret_cast<WGPUBindGroupLayout>(webgpuDevice->createBindGroupLayout(descriptor));
}

WGPUPipelineLayout procDeviceCreatePipelineLayout(WGPUDevice device, WGPUPipelineLayoutDescriptor const* descriptor)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return reinterpret_cast<WGPUPipelineLayout>(webgpuDevice->createPipelineLayout(descriptor));
}

WGPURenderPipeline procDeviceCreateRenderPipeline(WGPUDevice device, WGPURenderPipelineDescriptor const* descriptor)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return reinterpret_cast<WGPURenderPipeline>(webgpuDevice->createRenderPipeline(descriptor));
}

WGPUShaderModule procDeviceCreateShaderModule(WGPUDevice device, WGPUShaderModuleDescriptor const* descriptor)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return reinterpret_cast<WGPUShaderModule>(webgpuDevice->createShaderModule(descriptor));
}

void procSurfaceGetCurrentTexture(WGPUSurface surface, WGPUSurfaceTexture* surfaceTexture)
{
    WebGPUSurface* webgpuSurface = reinterpret_cast<WebGPUSurface*>(surface);
    return webgpuSurface->getCurrentTexture(surfaceTexture);
}

WGPUTextureView procTextureCreateView(WGPUTexture texture, WGPU_NULLABLE WGPUTextureViewDescriptor const* descriptor)
{
    WebGPUTexture* webgpuTexture = reinterpret_cast<WebGPUTexture*>(texture);
    return reinterpret_cast<WGPUTextureView>(webgpuTexture->createView(descriptor));
}

WGPUCommandEncoder procDeviceCreateCommandEncoder(WGPUDevice device, WGPU_NULLABLE WGPUCommandEncoderDescriptor const* descriptor)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return reinterpret_cast<WGPUCommandEncoder>(webgpuDevice->createCommandEncoder(descriptor));
}

WGPURenderPassEncoder procCommandEncoderBeginRenderPass(WGPUCommandEncoder commandEncoder, WGPURenderPassDescriptor const* descriptor)
{
    WebGPUCommandEncoder* webgpuCommandEncoder = reinterpret_cast<WebGPUCommandEncoder*>(commandEncoder);
    return reinterpret_cast<WGPURenderPassEncoder>(webgpuCommandEncoder->beginRenderPass(descriptor));
}

void procRenderPassEncoderSetPipeline(WGPURenderPassEncoder renderPassEncoder, WGPURenderPipeline pipeline)
{
    WebGPURenderPassEncoder* webgpuRenderPassEncoder = reinterpret_cast<WebGPURenderPassEncoder*>(renderPassEncoder);
    return webgpuRenderPassEncoder->setPipeline(pipeline);
}

void procRenderPassEncoderDraw(WGPURenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    WebGPURenderPassEncoder* webgpuRenderPassEncoder = reinterpret_cast<WebGPURenderPassEncoder*>(renderPassEncoder);
    return webgpuRenderPassEncoder->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void procRenderPassEncoderEnd(WGPURenderPassEncoder renderPassEncoder)
{
    WebGPURenderPassEncoder* webgpuRenderPassEncoder = reinterpret_cast<WebGPURenderPassEncoder*>(renderPassEncoder);
    return webgpuRenderPassEncoder->end();
}

void procRenderPassEncoderRelease(WGPURenderPassEncoder renderPassEncoder)
{
    WebGPURenderPassEncoder* webgpuRenderPassEncoder = reinterpret_cast<WebGPURenderPassEncoder*>(renderPassEncoder);
    return webgpuRenderPassEncoder->release();
}

WGPUCommandBuffer procCommandEncoderFinish(WGPUCommandEncoder commandEncoder, WGPU_NULLABLE WGPUCommandBufferDescriptor const* descriptor)
{
    WebGPUCommandEncoder* webgpuCommandEncoder = reinterpret_cast<WebGPUCommandEncoder*>(commandEncoder);
    return reinterpret_cast<WGPUCommandBuffer>(webgpuCommandEncoder->finish(descriptor));
}

void procQueueSubmit(WGPUQueue queue, size_t commandCount, WGPUCommandBuffer const* commands)
{
    WebGPUQueue* webgpuQueue = reinterpret_cast<WebGPUQueue*>(queue);
    return webgpuQueue->submit(commandCount, commands);
}

void procSurfacePresent(WGPUSurface surface)
{
    WebGPUSurface* webgpuSurface = reinterpret_cast<WebGPUSurface*>(surface);
    return webgpuSurface->present();
}

void procCommandBufferRelease(WGPUCommandBuffer commandBuffer)
{
    WebGPUCommandBuffer* webgpuCommandBuffer = reinterpret_cast<WebGPUCommandBuffer*>(commandBuffer);
    return webgpuCommandBuffer->release();
}

void procCommandEncoderRelease(WGPUCommandEncoder commandEncoder)
{
    WebGPUCommandEncoder* webgpuCommandEncoder = reinterpret_cast<WebGPUCommandEncoder*>(commandEncoder);
    return webgpuCommandEncoder->release();
}

void procTextureViewRelease(WGPUTextureView textureView)
{
    WebGPUTextureView* webgpuTextureView = reinterpret_cast<WebGPUTextureView*>(textureView);
    return webgpuTextureView->release();
}

void procTextureRelease(WGPUTexture texture)
{
    WebGPUTexture* webgpuTexture = reinterpret_cast<WebGPUTexture*>(texture);
    return webgpuTexture->release();
}

void procRenderPipelineRelease(WGPURenderPipeline renderPipeline)
{
    WebGPURenderPipeline* webgpuRenderPipeline = reinterpret_cast<WebGPURenderPipeline*>(renderPipeline);
    return webgpuRenderPipeline->release();
}

void procPipelineLayoutRelease(WGPUPipelineLayout pipelineLayout)
{
    WebGPUPipelineLayout* webgpuPipelineLayout = reinterpret_cast<WebGPUPipelineLayout*>(pipelineLayout);
    return webgpuPipelineLayout->release();
}

void procShaderModuleRelease(WGPUShaderModule shaderModule)
{
    WebGPUShaderModule* webgpuShaderModule = reinterpret_cast<WebGPUShaderModule*>(shaderModule);
    return webgpuShaderModule->release();
}

void procQueueRelease(WGPUQueue queue)
{
    WebGPUQueue* webgpuQueue = reinterpret_cast<WebGPUQueue*>(queue);
    return webgpuQueue->release();
}

void procDeviceDestroy(WGPUDevice device)
{
    // TODO
}

void procDeviceRelease(WGPUDevice device)
{
    WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(device);
    return webgpuDevice->release();
}

void procAdapterRelease(WGPUAdapter adapter)
{
    WebGPUAdapter* webgpuAdapter = reinterpret_cast<WebGPUAdapter*>(adapter);
    return webgpuAdapter->release();
}

void procSurfaceRelease(WGPUSurface surface)
{
    WebGPUSurface* webgpuSurface = reinterpret_cast<WebGPUSurface*>(surface);
    return webgpuSurface->release();
}

void procInstanceRelease(WGPUInstance instance)
{
    WebGPUInstance* webgpuInstance = reinterpret_cast<WebGPUInstance*>(instance);
    return webgpuInstance->release();
}

namespace
{

std::unordered_map<const char*, WGPUProc> sProcMap{
    { "wgpuCreateInstance", reinterpret_cast<WGPUProc>(procCreateInstance) },
    { "wgpuInstanceRequestAdapter", reinterpret_cast<WGPUProc>(procInstanceRequestAdapter) },
    { "wgpuInstanceCreateSurface", reinterpret_cast<WGPUProc>(procInstanceCreateSurface) },
    { "wgpuAdapterRequestDevice", reinterpret_cast<WGPUProc>(procAdapterRequestDevice) },
    { "wgpuDeviceGetQueue", reinterpret_cast<WGPUProc>(procDeviceGetQueue) },
    { "wgpuSurfaceGetCapabilities", reinterpret_cast<WGPUProc>(procSurfaceGetCapabilities) },
    { "wgpuSurfaceConfigure", reinterpret_cast<WGPUProc>(procSurfaceConfigure) },
    { "wgpuDeviceCreateBindGroup", reinterpret_cast<WGPUProc>(procDeviceCreateBindGroup) },
    { "wgpuDeviceCreateBindGroupLayout", reinterpret_cast<WGPUProc>(procDeviceCreateBindGroupLayout) },
    { "wgpuDeviceCreatePipelineLayout", reinterpret_cast<WGPUProc>(procDeviceCreatePipelineLayout) },
    { "wgpuDeviceCreateRenderPipeline", reinterpret_cast<WGPUProc>(procDeviceCreateRenderPipeline) },
    { "wpugDeviceCreateShaderModule", reinterpret_cast<WGPUProc>(procDeviceCreateShaderModule) },
    { "wgpuSurfaceGetCurrentTexture", reinterpret_cast<WGPUProc>(procSurfaceGetCurrentTexture) },
    { "wgpuTextureCreateView", reinterpret_cast<WGPUProc>(procTextureCreateView) },
    { "wgpuDeviceCreateCommandEncoder", reinterpret_cast<WGPUProc>(procDeviceCreateCommandEncoder) },
    { "wgpuCommandEncoderBeginRenderPass", reinterpret_cast<WGPUProc>(procCommandEncoderBeginRenderPass) },
    { "wgpuRenderPassEncoderSetPipeline", reinterpret_cast<WGPUProc>(procRenderPassEncoderSetPipeline) },
    { "wgpuRenderPassEncoderDraw", reinterpret_cast<WGPUProc>(procRenderPassEncoderDraw) },
    { "wgpuRenderPassEncoderEnd", reinterpret_cast<WGPUProc>(procRenderPassEncoderEnd) },
    { "wgpuRenderPassEncoderRelease", reinterpret_cast<WGPUProc>(procRenderPassEncoderRelease) },
    { "wgpuCommandEncoderFinish", reinterpret_cast<WGPUProc>(procCommandEncoderFinish) },
    { "wgpuQueueSubmit", reinterpret_cast<WGPUProc>(procQueueSubmit) },
    { "wgpuSurfacePresent", reinterpret_cast<WGPUProc>(procSurfacePresent) },
    { "wgpuCommandBufferRelease", reinterpret_cast<WGPUProc>(procCommandBufferRelease) },
    { "wgpuCommandEncoderRelease", reinterpret_cast<WGPUProc>(procCommandEncoderRelease) },
    { "wgpuTextureViewRelease", reinterpret_cast<WGPUProc>(procTextureViewRelease) },
    { "wgpuTextureRelease", reinterpret_cast<WGPUProc>(procTextureRelease) },
    { "wgpuRenderPipelineRelease", reinterpret_cast<WGPUProc>(procRenderPipelineRelease) },
    { "wgpuPipelineLayoutRelease", reinterpret_cast<WGPUProc>(procPipelineLayoutRelease) },
    { "wgpuShaderModuleRelease", reinterpret_cast<WGPUProc>(procShaderModuleRelease) },
    { "wgpuQueueRelease", reinterpret_cast<WGPUProc>(procQueueRelease) },
    { "wgpuDeviceDestroy", reinterpret_cast<WGPUProc>(procDeviceDestroy) },
    { "wgpuDeviceRelease", reinterpret_cast<WGPUProc>(procDeviceRelease) },
    { "wgpuAdapterRelease", reinterpret_cast<WGPUProc>(procAdapterRelease) },
    { "wgpuSurfaceRelease", reinterpret_cast<WGPUProc>(procSurfaceRelease) },
    { "wgpuInstanceRelease", reinterpret_cast<WGPUProc>(procInstanceRelease) },
};

} // namespace

WGPUProc procGetProcAddress(WGPUStringView procName)
{
    if (procName.data == nullptr)
    {
        return nullptr;
    }

    if (sProcMap.contains(procName.data))
    {
        return sProcMap[procName.data];
    }

    return nullptr;
}

} // namespace jipu