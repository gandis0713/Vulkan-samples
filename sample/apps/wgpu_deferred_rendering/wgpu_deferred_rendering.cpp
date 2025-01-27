#include "wgpu_deferred_rendering.h"

#include "file.h"
#include "image.h"

#include <cmath>
#include <random>
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUDeferredRenderingSample::WGPUDeferredRenderingSample(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
    m_imgui.emplace(this);
}

WGPUDeferredRenderingSample::~WGPUDeferredRenderingSample()
{
    finalizeContext();
}

void WGPUDeferredRenderingSample::init()
{
    WGPUSample::init();

    changeAPI(APIType::kDawn);
}

void WGPUDeferredRenderingSample::onBeforeUpdate()
{
    WGPUSample::onBeforeUpdate();

    static const char* modes[] = { "rendering", "gBuffers View" };

    recordImGui({ [&]() {
        windowImGui(
            "Deferred Rendering", { [&]() {
                ImGui::Combo("mode", &m_mode, modes, IM_ARRAYSIZE(modes));
                ImGui::SliderInt("numLights", &m_numLights, 1, 1024);
            } });
    } });
}

void WGPUDeferredRenderingSample::onUpdate()
{
    WGPUSample::onUpdate();

    // update
}

void WGPUDeferredRenderingSample::onDraw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

    WGPURenderPassDepthStencilAttachment depthStencilAttachment{};
    depthStencilAttachment.view = nullptr;
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthClearValue = 1.0f;

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    // TODO: draw

    drawImGui(commandEncoder, surfaceTextureView);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu.QueueSubmit(m_queue, 1, &commandBuffer);
    wgpu.SurfacePresent(m_surface);

    wgpu.CommandBufferRelease(commandBuffer);
    wgpu.CommandEncoderRelease(commandEncoder);
    wgpu.TextureViewRelease(surfaceTextureView);
    wgpu.TextureRelease(surfaceTexture.texture);
}

void WGPUDeferredRenderingSample::initializeContext()
{
    WGPUSample::initializeContext();
}

void WGPUDeferredRenderingSample::finalizeContext()
{
    // TODO: destroy resources

    WGPUSample::finalizeContext();
}

} // namespace jipu
