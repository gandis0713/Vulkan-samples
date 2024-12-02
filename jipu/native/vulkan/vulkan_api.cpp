#include "vulkan_api.h"

#include "jipu/common/dylib.h"
#include <spdlog/spdlog.h>

namespace jipu
{
bool VulkanAPI::loadInstanceProcs(DyLib* vulkanLib)
{
#define GET_GLOBAL_PROC(name)                                                          \
    name = reinterpret_cast<decltype(name)>(GetInstanceProcAddr(nullptr, "vk" #name)); \
    if (name == nullptr)                                                               \
    {                                                                                  \
        spdlog::error("Couldn't get instance proc vk{}", #name);                       \
        return false;                                                                  \
    }

    if (!vulkanLib->getProc(&GetInstanceProcAddr, "vkGetInstanceProcAddr"))
    {
        spdlog::error("Couldn't get vkGetInstanceProcAddr");
        return false;
    }

#if defined(VK_VERSION_1_0)
    GET_GLOBAL_PROC(CreateInstance);
    GET_GLOBAL_PROC(EnumerateInstanceExtensionProperties);
    GET_GLOBAL_PROC(EnumerateInstanceLayerProperties);
#endif

#if defined(VK_VERSION_1_1)
    // Is not available in Vulkan 1.0, so allow nullptr
    GET_GLOBAL_PROC(EnumerateInstanceVersion);
#endif

    return true;
}

bool VulkanAPI::loadInstanceProcs(VkInstance instance, const VulkanAdapterKnobs& instanceKnobs)
{
#define GET_INSTANCE_PROC(name)                                                         \
    name = reinterpret_cast<decltype(name)>(GetInstanceProcAddr(instance, "vk" #name)); \
    if (name == nullptr)                                                                \
    {                                                                                   \
        spdlog::error("Couldn't get instance proc vk{}", #name);                        \
        return false;                                                                   \
    }

    if (GetInstanceProcAddr == nullptr)
    {
        spdlog::error("vkGetInstanceProcAddr is nullptr.");
        return false;
    }

    // Load this proc first so that we can destroy the instance even if some other
    // GET_INSTANCE_PROC fails
    GET_INSTANCE_PROC(DestroyInstance);

#if defined(VK_VERSION_1_0)
    GET_INSTANCE_PROC(CreateDevice)
    GET_INSTANCE_PROC(DestroyDevice);
    GET_INSTANCE_PROC(DestroyInstance)
    GET_INSTANCE_PROC(EnumerateDeviceExtensionProperties)
    GET_INSTANCE_PROC(EnumerateDeviceLayerProperties)
    GET_INSTANCE_PROC(EnumeratePhysicalDevices)
    GET_INSTANCE_PROC(GetDeviceProcAddr)
    GET_INSTANCE_PROC(GetPhysicalDeviceFeatures)
    GET_INSTANCE_PROC(GetPhysicalDeviceFormatProperties)
    GET_INSTANCE_PROC(GetPhysicalDeviceImageFormatProperties)
    GET_INSTANCE_PROC(GetPhysicalDeviceMemoryProperties)
    GET_INSTANCE_PROC(GetPhysicalDeviceProperties)
    GET_INSTANCE_PROC(GetPhysicalDeviceQueueFamilyProperties)
    GET_INSTANCE_PROC(GetPhysicalDeviceSparseImageFormatProperties)
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
    // GET_INSTANCE_PROC(EnumeratePhysicalDeviceGroups)
    // GET_INSTANCE_PROC(GetPhysicalDeviceExternalBufferProperties)
    // GET_INSTANCE_PROC(GetPhysicalDeviceExternalFenceProperties)
    // GET_INSTANCE_PROC(GetPhysicalDeviceExternalSemaphoreProperties)
    // GET_INSTANCE_PROC(GetPhysicalDeviceFeatures2)
    // GET_INSTANCE_PROC(GetPhysicalDeviceFormatProperties2)
    // GET_INSTANCE_PROC(GetPhysicalDeviceImageFormatProperties2)
    // GET_INSTANCE_PROC(GetPhysicalDeviceMemoryProperties2)
    // GET_INSTANCE_PROC(GetPhysicalDeviceProperties2)
    // GET_INSTANCE_PROC(GetPhysicalDeviceQueueFamilyProperties2)
    // GET_INSTANCE_PROC(GetPhysicalDeviceSparseImageFormatProperties2)
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_VERSION_1_3)
    // GET_INSTANCE_PROC(GetPhysicalDeviceToolProperties)
#endif /* defined(VK_VERSION_1_3) */

#ifndef NDEBUG
    if (instanceKnobs.debugReport)
    {
        GET_INSTANCE_PROC(CreateDebugReportCallbackEXT);
        GET_INSTANCE_PROC(DebugReportMessageEXT);
        GET_INSTANCE_PROC(DestroyDebugReportCallbackEXT);
    }

    if (instanceKnobs.debugUtils)
    {
        GET_INSTANCE_PROC(CmdBeginDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(CmdEndDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(CmdInsertDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(CreateDebugUtilsMessengerEXT);
        GET_INSTANCE_PROC(DestroyDebugUtilsMessengerEXT);
        GET_INSTANCE_PROC(QueueBeginDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(QueueEndDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(QueueInsertDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(SetDebugUtilsObjectNameEXT);
        GET_INSTANCE_PROC(SetDebugUtilsObjectTagEXT);
        GET_INSTANCE_PROC(SubmitDebugUtilsMessageEXT);
    }
#endif

    // // Vulkan 1.1 is not required to report promoted extensions from 1.0
    // if (instanceKnobs.externalMemoryCapabilities ||
    //     instanceKnobs.apiVersion >= VK_MAKE_VERSION(1, 1, 0))
    // {
    //     GET_INSTANCE_PROC(GetPhysicalDeviceExternalBufferPropertiesKHR);
    // }

    // if (instanceKnobs.externalSemaphoreCapabilities ||
    //     instanceKnobs.apiVersion >= VK_MAKE_VERSION(1, 1, 0))
    // {
    //     GET_INSTANCE_PROC(GetPhysicalDeviceExternalSemaphorePropertiesKHR);
    // }

    // if (instanceKnobs.getPhysicalDeviceProperties2 ||
    //     instanceKnobs.apiVersion >= VK_MAKE_VERSION(1, 1, 0))
    // {
    //     GET_INSTANCE_PROC(GetPhysicalDeviceFeatures2KHR);
    //     GET_INSTANCE_PROC(GetPhysicalDeviceProperties2KHR);
    //     GET_INSTANCE_PROC(GetPhysicalDeviceFormatProperties2KHR);
    //     GET_INSTANCE_PROC(GetPhysicalDeviceImageFormatProperties2KHR);
    //     GET_INSTANCE_PROC(GetPhysicalDeviceQueueFamilyProperties2KHR);
    //     GET_INSTANCE_PROC(GetPhysicalDeviceMemoryProperties2KHR);
    //     GET_INSTANCE_PROC(GetPhysicalDeviceSparseImageFormatProperties2KHR);
    // }

    if (instanceKnobs.surface)
    {
        GET_INSTANCE_PROC(DestroySurfaceKHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceSupportKHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceCapabilitiesKHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceFormatsKHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfacePresentModesKHR);
    }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    if (instanceKnobs.androidSurface)
    {
        GET_INSTANCE_PROC(CreateAndroidSurfaceKHR);
    }
#endif

#if defined(VK_USE_PLATFORM_MACOS_MVK)
    if (instanceKnobs.macosSurface)
    {
        GET_INSTANCE_PROC(CreateMacOSSurfaceMVK);
    }
#endif

#if defined(VK_USE_PLATFORM_METAL_EXT)
    if (instanceKnobs.metalSurface)
    {
        GET_INSTANCE_PROC(CreateMetalSurfaceEXT);
    }
#endif

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (instanceKnobs.win32Surface)
    {
        GET_INSTANCE_PROC(CreateWin32SurfaceKHR);
    }
#endif

    // #ifdef VK_USE_PLATFORM_FUCHSIA
    //     if (instanceKnobs.fuchsiaImagePipeSurface)
    //     {
    //         GET_INSTANCE_PROC(CreateImagePipeSurfaceFUCHSIA);
    //     }
    // #endif

    return true;
}

bool VulkanAPI::loadDeviceProcs(VkDevice device, const VulkanDeviceKnobs& deviceKnobs)
{
#define GET_DEVICE_PROC(name)                                                       \
    name = reinterpret_cast<decltype(name)>(GetDeviceProcAddr(device, "vk" #name)); \
    if (name == nullptr)                                                            \
    {                                                                               \
        spdlog::error("Couldn't get device proc vk{}", #name);                      \
        return false;                                                               \
    }

#if defined(VK_VERSION_1_0)
    GET_DEVICE_PROC(AllocateCommandBuffers);
    GET_DEVICE_PROC(AllocateDescriptorSets);
    GET_DEVICE_PROC(AllocateMemory);
    GET_DEVICE_PROC(BeginCommandBuffer);
    GET_DEVICE_PROC(BindBufferMemory);
    GET_DEVICE_PROC(BindImageMemory);
    GET_DEVICE_PROC(CmdBeginQuery);
    GET_DEVICE_PROC(CmdBeginRenderPass);
    GET_DEVICE_PROC(CmdBindDescriptorSets);
    GET_DEVICE_PROC(CmdBindIndexBuffer);
    GET_DEVICE_PROC(CmdBindPipeline);
    GET_DEVICE_PROC(CmdBindVertexBuffers);
    GET_DEVICE_PROC(CmdBlitImage);
    GET_DEVICE_PROC(CmdClearAttachments);
    GET_DEVICE_PROC(CmdClearColorImage);
    GET_DEVICE_PROC(CmdClearDepthStencilImage);
    GET_DEVICE_PROC(CmdCopyBuffer);
    GET_DEVICE_PROC(CmdCopyBufferToImage);
    GET_DEVICE_PROC(CmdCopyImage);
    GET_DEVICE_PROC(CmdCopyImageToBuffer);
    GET_DEVICE_PROC(CmdCopyQueryPoolResults);
    GET_DEVICE_PROC(CmdDispatch);
    GET_DEVICE_PROC(CmdDispatchIndirect);
    GET_DEVICE_PROC(CmdDraw);
    GET_DEVICE_PROC(CmdDrawIndexed);
    GET_DEVICE_PROC(CmdDrawIndexedIndirect);
    GET_DEVICE_PROC(CmdDrawIndirect);
    GET_DEVICE_PROC(CmdEndQuery);
    GET_DEVICE_PROC(CmdEndRenderPass);
    GET_DEVICE_PROC(CmdExecuteCommands);
    GET_DEVICE_PROC(CmdFillBuffer);
    GET_DEVICE_PROC(CmdNextSubpass);
    GET_DEVICE_PROC(CmdPipelineBarrier);
    GET_DEVICE_PROC(CmdPushConstants);
    GET_DEVICE_PROC(CmdResetEvent);
    GET_DEVICE_PROC(CmdResetQueryPool);
    GET_DEVICE_PROC(CmdResolveImage);
    GET_DEVICE_PROC(CmdSetBlendConstants);
    GET_DEVICE_PROC(CmdSetDepthBias);
    GET_DEVICE_PROC(CmdSetDepthBounds);
    GET_DEVICE_PROC(CmdSetEvent);
    GET_DEVICE_PROC(CmdSetLineWidth);
    GET_DEVICE_PROC(CmdSetScissor);
    GET_DEVICE_PROC(CmdSetStencilCompareMask);
    GET_DEVICE_PROC(CmdSetStencilReference);
    GET_DEVICE_PROC(CmdSetStencilWriteMask);
    GET_DEVICE_PROC(CmdSetViewport);
    GET_DEVICE_PROC(CmdUpdateBuffer);
    GET_DEVICE_PROC(CmdWaitEvents);
    GET_DEVICE_PROC(CmdWriteTimestamp);
    GET_DEVICE_PROC(CreateBuffer);
    GET_DEVICE_PROC(CreateBufferView);
    GET_DEVICE_PROC(CreateCommandPool);
    GET_DEVICE_PROC(CreateComputePipelines);
    GET_DEVICE_PROC(CreateDescriptorPool);
    GET_DEVICE_PROC(CreateDescriptorSetLayout);
    GET_DEVICE_PROC(CreateEvent);
    GET_DEVICE_PROC(CreateFence);
    GET_DEVICE_PROC(CreateFramebuffer);
    GET_DEVICE_PROC(CreateGraphicsPipelines);
    GET_DEVICE_PROC(CreateImage);
    GET_DEVICE_PROC(CreateImageView);
    GET_DEVICE_PROC(CreatePipelineCache);
    GET_DEVICE_PROC(CreatePipelineLayout);
    GET_DEVICE_PROC(CreateQueryPool);
    GET_DEVICE_PROC(CreateRenderPass);
    GET_DEVICE_PROC(CreateSampler);
    GET_DEVICE_PROC(CreateSemaphore);
    GET_DEVICE_PROC(CreateShaderModule);
    GET_DEVICE_PROC(DestroyBuffer);
    GET_DEVICE_PROC(DestroyBufferView);
    GET_DEVICE_PROC(DestroyCommandPool);
    GET_DEVICE_PROC(DestroyDescriptorPool);
    GET_DEVICE_PROC(DestroyDescriptorSetLayout);
    GET_DEVICE_PROC(DestroyEvent);
    GET_DEVICE_PROC(DestroyFence);
    GET_DEVICE_PROC(DestroyFramebuffer);
    GET_DEVICE_PROC(DestroyImage);
    GET_DEVICE_PROC(DestroyImageView);
    GET_DEVICE_PROC(DestroyPipeline);
    GET_DEVICE_PROC(DestroyPipelineCache);
    GET_DEVICE_PROC(DestroyPipelineLayout);
    GET_DEVICE_PROC(DestroyQueryPool);
    GET_DEVICE_PROC(DestroyRenderPass);
    GET_DEVICE_PROC(DestroySampler);
    GET_DEVICE_PROC(DestroySemaphore);
    GET_DEVICE_PROC(DestroyShaderModule);
    GET_DEVICE_PROC(DeviceWaitIdle);
    GET_DEVICE_PROC(EndCommandBuffer);
    GET_DEVICE_PROC(FlushMappedMemoryRanges);
    GET_DEVICE_PROC(FreeCommandBuffers);
    GET_DEVICE_PROC(FreeDescriptorSets);
    GET_DEVICE_PROC(FreeMemory);
    GET_DEVICE_PROC(GetBufferMemoryRequirements);
    GET_DEVICE_PROC(GetDeviceMemoryCommitment);
    GET_DEVICE_PROC(GetDeviceQueue);
    GET_DEVICE_PROC(GetEventStatus);
    GET_DEVICE_PROC(GetFenceStatus);
    GET_DEVICE_PROC(GetImageMemoryRequirements);
    GET_DEVICE_PROC(GetImageSparseMemoryRequirements);
    GET_DEVICE_PROC(GetImageSubresourceLayout);
    GET_DEVICE_PROC(GetPipelineCacheData);
    GET_DEVICE_PROC(GetQueryPoolResults);
    GET_DEVICE_PROC(GetRenderAreaGranularity);
    GET_DEVICE_PROC(InvalidateMappedMemoryRanges);
    GET_DEVICE_PROC(MapMemory);
    GET_DEVICE_PROC(MergePipelineCaches);
    GET_DEVICE_PROC(QueueBindSparse);
    GET_DEVICE_PROC(QueueSubmit);
    GET_DEVICE_PROC(QueueWaitIdle);
    GET_DEVICE_PROC(ResetCommandBuffer);
    GET_DEVICE_PROC(ResetCommandPool);
    GET_DEVICE_PROC(ResetDescriptorPool);
    GET_DEVICE_PROC(ResetEvent);
    GET_DEVICE_PROC(ResetFences);
    GET_DEVICE_PROC(SetEvent);
    GET_DEVICE_PROC(UnmapMemory);
    GET_DEVICE_PROC(UpdateDescriptorSets);
    GET_DEVICE_PROC(WaitForFences);
#endif
#if defined(VK_VERSION_1_1)
    // GET_DEVICE_PROC(BindBufferMemory2)
    // GET_DEVICE_PROC(BindImageMemory2)
    // GET_DEVICE_PROC(CmdDispatchBase)
    // GET_DEVICE_PROC(CmdSetDeviceMask)
    // GET_DEVICE_PROC(CreateDescriptorUpdateTemplate)
    // GET_DEVICE_PROC(CreateSamplerYcbcrConversion)
    // GET_DEVICE_PROC(DestroyDescriptorUpdateTemplate)
    // GET_DEVICE_PROC(DestroySamplerYcbcrConversion)
    // GET_DEVICE_PROC(GetBufferMemoryRequirements2)
    // GET_DEVICE_PROC(GetDescriptorSetLayoutSupport)
    // GET_DEVICE_PROC(GetDeviceGroupPeerMemoryFeatures)
    // GET_DEVICE_PROC(GetDeviceQueue2)
    // GET_DEVICE_PROC(GetImageMemoryRequirements2)
    // GET_DEVICE_PROC(GetImageSparseMemoryRequirements2)
    // GET_DEVICE_PROC(TrimCommandPool)
    // GET_DEVICE_PROC(UpdateDescriptorSetWithTemplate)

#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_VERSION_1_2)
    // GET_DEVICE_PROC(CmdBeginRenderPass2)
    // GET_DEVICE_PROC(CmdDrawIndexedIndirectCount)
    // GET_DEVICE_PROC(CmdDrawIndirectCount)
    // GET_DEVICE_PROC(CmdEndRenderPass2)
    // GET_DEVICE_PROC(CmdNextSubpass2)
    // GET_DEVICE_PROC(CreateRenderPass2)
    // GET_DEVICE_PROC(GetBufferDeviceAddress)
    // GET_DEVICE_PROC(GetBufferOpaqueCaptureAddress)
    // GET_DEVICE_PROC(GetDeviceMemoryOpaqueCaptureAddress)
    // GET_DEVICE_PROC(GetSemaphoreCounterValue)
    // GET_DEVICE_PROC(ResetQueryPool)
    // GET_DEVICE_PROC(SignalSemaphore)
    // GET_DEVICE_PROC(WaitSemaphores)

#endif /* defined(VK_VERSION_1_2) */
#if defined(VK_VERSION_1_3)
    // GET_DEVICE_PROC(CmdBeginRendering);
    // GET_DEVICE_PROC(CmdBindVertexBuffers2);
    // GET_DEVICE_PROC(CmdBlitImage2);
    // GET_DEVICE_PROC(CmdCopyBuffer2);
    // GET_DEVICE_PROC(CmdCopyBufferToImage2);
    // GET_DEVICE_PROC(CmdCopyImage2);
    // GET_DEVICE_PROC(CmdCopyImageToBuffer2);
    // GET_DEVICE_PROC(CmdEndRendering);
    // GET_DEVICE_PROC(CmdPipelineBarrier2);
    // GET_DEVICE_PROC(CmdResetEvent2);
    // GET_DEVICE_PROC(CmdResolveImage2);
    // GET_DEVICE_PROC(CmdSetCullMode);
    // GET_DEVICE_PROC(CmdSetDepthBiasEnable);
    // GET_DEVICE_PROC(CmdSetDepthBoundsTestEnable);
    // GET_DEVICE_PROC(CmdSetDepthCompareOp);
    // GET_DEVICE_PROC(CmdSetDepthTestEnable);
    // GET_DEVICE_PROC(CmdSetDepthWriteEnable);
    // GET_DEVICE_PROC(CmdSetEvent2);
    // GET_DEVICE_PROC(CmdSetFrontFace);
    // GET_DEVICE_PROC(CmdSetPrimitiveRestartEnable);
    // GET_DEVICE_PROC(CmdSetPrimitiveTopology);
    // GET_DEVICE_PROC(CmdSetRasterizerDiscardEnable);
    // GET_DEVICE_PROC(CmdSetScissorWithCount);
    // GET_DEVICE_PROC(CmdSetStencilOp);
    // GET_DEVICE_PROC(CmdSetStencilTestEnable);
    // GET_DEVICE_PROC(CmdSetViewportWithCount);
    // GET_DEVICE_PROC(CmdWaitEvents2);
    // GET_DEVICE_PROC(CmdWriteTimestamp2);
    // GET_DEVICE_PROC(CreatePrivateDataSlot);
    // GET_DEVICE_PROC(DestroyPrivateDataSlot);
    // GET_DEVICE_PROC(GetDeviceBufferMemoryRequirements);
    // GET_DEVICE_PROC(GetDeviceImageMemoryRequirements);
    // GET_DEVICE_PROC(GetDeviceImageSparseMemoryRequirements);
    // GET_DEVICE_PROC(GetPhysicalDeviceToolProperties);
    // GET_DEVICE_PROC(GetPrivateData);
    // GET_DEVICE_PROC(QueueSubmit2);
    // GET_DEVICE_PROC(SetPrivateData);
#endif /* defined(VK_VERSION_1_3) */

    if (deviceKnobs.swapchain)
    {
        GET_DEVICE_PROC(CreateSwapchainKHR);
        GET_DEVICE_PROC(DestroySwapchainKHR);
        GET_DEVICE_PROC(GetSwapchainImagesKHR);
        GET_DEVICE_PROC(AcquireNextImageKHR);
        GET_DEVICE_PROC(QueuePresentKHR);
    }
    // if (deviceKnobs.debugMarker)
    // {
    //     GET_DEVICE_PROC(CmdDebugMarkerBeginEXT);
    //     GET_DEVICE_PROC(CmdDebugMarkerEndEXT);
    //     GET_DEVICE_PROC(CmdDebugMarkerInsertEXT);
    // }

    // if (deviceKnobs.externalMemoryFD)
    // {
    //     GET_DEVICE_PROC(GetMemoryFdKHR);
    //     GET_DEVICE_PROC(GetMemoryFdPropertiesKHR);
    // }

    // if (deviceKnobs.externalSemaphoreFD)
    // {
    //     GET_DEVICE_PROC(ImportSemaphoreFdKHR);
    //     GET_DEVICE_PROC(GetSemaphoreFdKHR);
    // }

    // #if VK_USE_PLATFORM_FUCHSIA
    //     if (deviceKnobs.externalMemoryZirconHandle)
    //     {
    //         GET_DEVICE_PROC(GetMemoryZirconHandleFUCHSIA);
    //         GET_DEVICE_PROC(GetMemoryZirconHandlePropertiesFUCHSIA);
    //     }

    //     if (deviceKnobs.externalSemaphoreZirconHandle)
    //     {
    //         GET_DEVICE_PROC(ImportSemaphoreZirconHandleFUCHSIA);
    //         GET_DEVICE_PROC(GetSemaphoreZirconHandleFUCHSIA);
    //     }
    // #endif

    return true;
}

} // namespace jipu