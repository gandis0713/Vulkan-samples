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

    createVertexBuffer();
    createIndexBuffer();
}

void WGPUDeferredRenderingSample::finalizeContext()
{
    if (m_vertexBuffer)
    {
        wgpu.BufferRelease(m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }

    if (m_indexBuffer)
    {
        wgpu.BufferRelease(m_indexBuffer);
        m_indexBuffer = nullptr;
    }

    WGPUSample::finalizeContext();
}

void WGPUDeferredRenderingSample::createVertexBuffer()
{
    // Create the model vertex buffer.
    const uint32_t vertexStride = 8;
    WGPUBufferDescriptor vertexBufferDescriptor{};
    vertexBufferDescriptor.size = m_dragonMesh.positions.size() * vertexStride * sizeof(float);
    vertexBufferDescriptor.usage = WGPUBufferUsage_Vertex;
    vertexBufferDescriptor.mappedAtCreation = true;

    m_vertexBuffer = wgpu.DeviceCreateBuffer(m_device, &vertexBufferDescriptor);
    assert(m_vertexBuffer);

    void* mappedVertexPtr = wgpu.BufferGetMappedRange(m_vertexBuffer, 0, vertexBufferDescriptor.size);
    auto vertexBuffer = reinterpret_cast<float*>(mappedVertexPtr);
    for (auto i = 0; i < m_dragonMesh.positions.size(); ++i)
    {
        memcpy(vertexBuffer + vertexStride * i, &m_dragonMesh.positions[i], 3 * sizeof(float));
        memcpy(vertexBuffer + vertexStride * i + 3, &m_dragonMesh.normals[i], 3 * sizeof(float));
        memcpy(vertexBuffer + vertexStride * i + 6, &m_dragonMesh.uvs[i], 2 * sizeof(float));
    }

    wgpu.BufferUnmap(m_vertexBuffer);
}

void WGPUDeferredRenderingSample::createIndexBuffer()
{
    // Create the model index buffer.
    const uint32_t indexCount = m_dragonMesh.triangles.size() * 3;
    WGPUBufferDescriptor indexBufferDescriptor{};
    indexBufferDescriptor.size = indexCount * sizeof(uint32_t);
    indexBufferDescriptor.usage = WGPUBufferUsage_Index;
    indexBufferDescriptor.mappedAtCreation = true;

    m_indexBuffer = wgpu.DeviceCreateBuffer(m_device, &indexBufferDescriptor);
    assert(m_indexBuffer);

    {
        void* mappedIndexPtr = wgpu.BufferGetMappedRange(m_indexBuffer, 0, indexBufferDescriptor.size);
        auto indexBuffer = reinterpret_cast<uint32_t*>(mappedIndexPtr);
        for (auto i = 0; i < m_dragonMesh.triangles.size(); ++i)
        {
            memcpy(indexBuffer + 3 * i, &m_dragonMesh.triangles[i], 3 * sizeof(uint32_t));
        }

        wgpu.BufferUnmap(m_indexBuffer);
    }
}

} // namespace jipu
