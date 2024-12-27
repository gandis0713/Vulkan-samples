#include "webgpu_api.h"

#include "jipu/common/dylib.h"

#include <spdlog/spdlog.h>

namespace jipu
{

bool WebGPUAPI::loadProcs(DyLib* webgpuLib)
{
#define GET_PROC(name)                                                                 \
    std::string name##Str = "wgpu" #name;                                              \
    WGPUStringView name##View{ .data = name##Str.data(), .length = name##Str.size() }; \
    name = reinterpret_cast<decltype(name)>(GetProcAddress(name##View));               \
    if (name == nullptr)                                                               \
    {                                                                                  \
        spdlog::error("Couldn't get proc wgpu{}", #name);                              \
        return false;                                                                  \
    }

    if (!webgpuLib->getProc(&GetProcAddress, "wgpuGetProcAddress"))
    {
        spdlog::error("Couldn't get wgpuGetProcAddress");
        return false;
    }

    GET_PROC(CreateInstance);
    // GET_PROC(GetInstanceFeatures);
    // GET_PROC(AdapterGetFeatures);
    // GET_PROC(AdapterGetInfo);
    // GET_PROC(AdapterGetLimits);
    // GET_PROC(AdapterHasFeature);
    // GET_PROC(AdapterRequestDevice);
    GET_PROC(AdapterRequestDevice2);
    // GET_PROC(AdapterAddRef);
    GET_PROC(AdapterRelease);
    // GET_PROC(AdapterInfoFreeMembers);
    // GET_PROC(BindGroupSetLabel);
    // GET_PROC(BindGroupAddRef);
    GET_PROC(BindGroupRelease);
    // GET_PROC(BindGroupLayoutSetLabel);
    // GET_PROC(BindGroupLayoutAddRef);
    GET_PROC(BindGroupLayoutRelease);
    // GET_PROC(BufferDestroy);
    // GET_PROC(BufferGetConstMappedRange);
    // GET_PROC(BufferGetMapState);
    GET_PROC(BufferGetMappedRange);
    GET_PROC(BufferGetSize);
    // GET_PROC(BufferGetUsage);
    // GET_PROC(BufferMapAsync);
    // GET_PROC(BufferSetLabel);
    GET_PROC(BufferUnmap);
    // GET_PROC(BufferAddRef);
    GET_PROC(BufferRelease);
    // GET_PROC(CommandBufferSetLabel);
    // GET_PROC(CommandBufferAddRef);
    GET_PROC(CommandBufferRelease);
    // GET_PROC(CommandEncoderBeginComputePass);
    GET_PROC(CommandEncoderBeginRenderPass);
    // GET_PROC(CommandEncoderClearBuffer);
    // GET_PROC(CommandEncoderCopyBufferToBuffer);
    // GET_PROC(CommandEncoderCopyBufferToTexture);
    // GET_PROC(CommandEncoderCopyTextureToBuffer);
    // GET_PROC(CommandEncoderCopyTextureToTexture);
    GET_PROC(CommandEncoderFinish);
    // GET_PROC(CommandEncoderInsertDebugMarker);
    // GET_PROC(CommandEncoderPopDebugGroup);
    // GET_PROC(CommandEncoderPushDebugGroup);
    // GET_PROC(CommandEncoderResolveQuerySet);
    // GET_PROC(CommandEncoderSetLabel);
    // GET_PROC(CommandEncoderWriteTimestamp);
    // GET_PROC(CommandEncoderAddRef);
    GET_PROC(CommandEncoderRelease);
    // GET_PROC(ComputePassEncoderDispatchWorkgroups);
    // GET_PROC(ComputePassEncoderDispatchWorkgroupsIndirect);
    // GET_PROC(ComputePassEncoderEnd);
    // GET_PROC(ComputePassEncoderInsertDebugMarker);
    // GET_PROC(ComputePassEncoderPopDebugGroup);
    // GET_PROC(ComputePassEncoderPushDebugGroup);
    // GET_PROC(ComputePassEncoderSetBindGroup);
    // GET_PROC(ComputePassEncoderSetLabel);
    // GET_PROC(ComputePassEncoderSetPipeline);
    // GET_PROC(ComputePassEncoderAddRef);
    // GET_PROC(ComputePassEncoderRelease);
    // GET_PROC(ComputePipelineGetBindGroupLayout);
    // GET_PROC(ComputePipelineSetLabel);
    // GET_PROC(ComputePipelineAddRef);
    // GET_PROC(ComputePipelineRelease);
    GET_PROC(DeviceCreateBindGroup);
    GET_PROC(DeviceCreateBindGroupLayout);
    GET_PROC(DeviceCreateBuffer);
    GET_PROC(DeviceCreateCommandEncoder);
    // GET_PROC(DeviceCreateComputePipeline);
    // GET_PROC(DeviceCreateComputePipelineAsync);
    GET_PROC(DeviceCreatePipelineLayout);
    // GET_PROC(DeviceCreateQuerySet);
    GET_PROC(DeviceCreateRenderBundleEncoder);
    GET_PROC(DeviceCreateRenderPipeline);
    // GET_PROC(DeviceCreateRenderPipelineAsync);
    GET_PROC(DeviceCreateSampler);
    GET_PROC(DeviceCreateShaderModule);
    GET_PROC(DeviceCreateTexture);
    GET_PROC(DeviceDestroy);
    // GET_PROC(DeviceGetFeatures);
    // GET_PROC(DeviceGetLimits);
    GET_PROC(DeviceGetQueue);
    // GET_PROC(DeviceHasFeature);
    // GET_PROC(DevicePopErrorScope);
    // GET_PROC(DevicePushErrorScope);
    // GET_PROC(DeviceSetLabel);
    // GET_PROC(DeviceAddRef);
    GET_PROC(DeviceRelease);
    GET_PROC(InstanceCreateSurface);
    // GET_PROC(InstanceHasWGSLLanguageFeature);
    GET_PROC(InstanceProcessEvents);
    // GET_PROC(InstanceRequestAdapter);
    GET_PROC(InstanceRequestAdapter2);
    GET_PROC(InstanceWaitAny);
    // GET_PROC(InstanceAddRef);
    GET_PROC(InstanceRelease);
    // GET_PROC(PipelineLayoutSetLabel);
    // GET_PROC(PipelineLayoutAddRef);
    GET_PROC(PipelineLayoutRelease);
    // GET_PROC(QuerySetDestroy);
    // GET_PROC(QuerySetGetCount);
    // GET_PROC(QuerySetGetType);
    // GET_PROC(QuerySetSetLabel);
    // GET_PROC(QuerySetAddRef);
    // GET_PROC(QuerySetRelease);
    // GET_PROC(QueueOnSubmittedWorkDone);
    GET_PROC(QueueOnSubmittedWorkDone2);
    // GET_PROC(QueueSetLabel);
    GET_PROC(QueueSubmit);
    GET_PROC(QueueWriteBuffer);
    GET_PROC(QueueWriteTexture);
    // GET_PROC(QueueAddRef);
    GET_PROC(QueueRelease);
    // GET_PROC(RenderBundleSetLabel);
    // GET_PROC(RenderBundleAddRef);
    // GET_PROC(RenderBundleRelease);
    GET_PROC(RenderBundleEncoderDraw);
    GET_PROC(RenderBundleEncoderDrawIndexed);
    // GET_PROC(RenderBundleEncoderDrawIndexedIndirect);
    // GET_PROC(RenderBundleEncoderDrawIndirect);
    GET_PROC(RenderBundleEncoderFinish);
    // GET_PROC(RenderBundleEncoderInsertDebugMarker);
    // GET_PROC(RenderBundleEncoderPopDebugGroup);
    // GET_PROC(RenderBundleEncoderPushDebugGroup);
    GET_PROC(RenderBundleEncoderSetBindGroup);
    GET_PROC(RenderBundleEncoderSetIndexBuffer);
    // GET_PROC(RenderBundleEncoderSetLabel);
    GET_PROC(RenderBundleEncoderSetPipeline);
    GET_PROC(RenderBundleEncoderSetVertexBuffer);
    // GET_PROC(RenderBundleEncoderAddRef);
    // GET_PROC(RenderBundleEncoderRelease);
    // GET_PROC(RenderPassEncoderBeginOcclusionQuery);
    GET_PROC(RenderPassEncoderDraw);
    GET_PROC(RenderPassEncoderDrawIndexed);
    // GET_PROC(RenderPassEncoderDrawIndexedIndirect);
    // GET_PROC(RenderPassEncoderDrawIndirect);
    GET_PROC(RenderPassEncoderEnd);
    // GET_PROC(RenderPassEncoderEndOcclusionQuery);
    // GET_PROC(RenderPassEncoderExecuteBundles);
    // GET_PROC(RenderPassEncoderInsertDebugMarker);
    // GET_PROC(RenderPassEncoderPopDebugGroup);
    // GET_PROC(RenderPassEncoderPushDebugGroup);
    GET_PROC(RenderPassEncoderSetBindGroup);
    // GET_PROC(RenderPassEncoderSetBlendConstant);
    GET_PROC(RenderPassEncoderSetIndexBuffer);
    // GET_PROC(RenderPassEncoderSetLabel);
    GET_PROC(RenderPassEncoderSetPipeline);
    GET_PROC(RenderPassEncoderSetScissorRect);
    // GET_PROC(RenderPassEncoderSetStencilReference);
    GET_PROC(RenderPassEncoderSetVertexBuffer);
    GET_PROC(RenderPassEncoderSetViewport);
    // GET_PROC(RenderPassEncoderAddRef);
    GET_PROC(RenderPassEncoderRelease);
    // GET_PROC(RenderPipelineGetBindGroupLayout);
    // GET_PROC(RenderPipelineSetLabel);
    // GET_PROC(RenderPipelineAddRef);
    GET_PROC(RenderPipelineRelease);
    // GET_PROC(SamplerSetLabel);
    // GET_PROC(SamplerAddRef);
    GET_PROC(SamplerRelease);
    // GET_PROC(ShaderModuleGetCompilationInfo);
    // GET_PROC(ShaderModuleSetLabel);
    // GET_PROC(ShaderModuleAddRef);
    GET_PROC(ShaderModuleRelease);
    // GET_PROC(SupportedFeaturesFreeMembers);
    GET_PROC(SurfaceConfigure);
    GET_PROC(SurfaceGetCapabilities);
    GET_PROC(SurfaceGetCurrentTexture);
    GET_PROC(SurfacePresent);
    // GET_PROC(SurfaceSetLabel);
    // GET_PROC(SurfaceUnconfigure);
    // GET_PROC(SurfaceAddRef);
    GET_PROC(SurfaceRelease);
    // GET_PROC(SurfaceCapabilitiesFreeMembers);
    GET_PROC(TextureCreateView);
    // GET_PROC(TextureDestroy);
    // GET_PROC(TextureGetDepthOrArrayLayers);
    // GET_PROC(TextureGetDimension);
    // GET_PROC(TextureGetFormat);
    // GET_PROC(TextureGetHeight);
    // GET_PROC(TextureGetMipLevelCount);
    // GET_PROC(TextureGetSampleCount);
    // GET_PROC(TextureGetUsage);
    // GET_PROC(TextureGetWidth);
    // GET_PROC(TextureSetLabel);
    // GET_PROC(TextureAddRef);
    GET_PROC(TextureRelease);
    // GET_PROC(TextureViewSetLabel);
    // GET_PROC(TextureViewAddRef);
    GET_PROC(TextureViewRelease);

    return true;
}

} // namespace jipu