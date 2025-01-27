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
    createFloat16Texture();
    createAlbedoTexture();
    createDepthTexture();
    createFloat16TextureView();
    createAlbedoTextureView();
    createDepthTextureView();
    createShaderModules();
    createGBufferBindGroupLayout();
    createGBufferBindGroup();
    createGBufferPipelineLayout();
    createGBufferRenderPipeline();
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

    if (m_float16Texture)
    {
        wgpu.TextureRelease(m_float16Texture);
        m_float16Texture = nullptr;
    }

    if (m_albedoTexture)
    {
        wgpu.TextureRelease(m_albedoTexture);
        m_albedoTexture = nullptr;
    }

    if (m_depthTexture)
    {
        wgpu.TextureRelease(m_depthTexture);
        m_depthTexture = nullptr;
    }

    if (m_float16TextureView)
    {
        wgpu.TextureViewRelease(m_float16TextureView);
        m_float16TextureView = nullptr;
    }

    if (m_albedoTextureView)
    {
        wgpu.TextureViewRelease(m_albedoTextureView);
        m_albedoTextureView = nullptr;
    }

    if (m_depthTextureView)
    {
        wgpu.TextureViewRelease(m_depthTextureView);
        m_depthTextureView = nullptr;
    }

    if (m_fragmentDeferredRenderingShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragmentDeferredRenderingShaderModule);
        m_fragmentDeferredRenderingShaderModule = nullptr;
    }

    if (m_fragmentGBufferDebugViewShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragmentGBufferDebugViewShaderModule);
        m_fragmentGBufferDebugViewShaderModule = nullptr;
    }

    if (m_fragmentWriteGBuffersShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragmentWriteGBuffersShaderModule);
        m_fragmentWriteGBuffersShaderModule = nullptr;
    }

    if (m_vertexTextureQuadShaderModule)
    {
        wgpu.ShaderModuleRelease(m_vertexTextureQuadShaderModule);
        m_vertexTextureQuadShaderModule = nullptr;
    }

    if (m_vertexWriteGBuffersShaderModule)
    {
        wgpu.ShaderModuleRelease(m_vertexWriteGBuffersShaderModule);
        m_vertexWriteGBuffersShaderModule = nullptr;
    }

    if (m_lightUpdateShaderModule)
    {
        wgpu.ShaderModuleRelease(m_lightUpdateShaderModule);
        m_lightUpdateShaderModule = nullptr;
    }

    if (m_gBufferBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_gBufferBindGroupLayout);
        m_gBufferBindGroupLayout = nullptr;
    }

    if (m_gBufferBindGroup)
    {
        wgpu.BindGroupRelease(m_gBufferBindGroup);
        m_gBufferBindGroup = nullptr;
    }

    if (m_gBufferPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_gBufferPipelineLayout);
        m_gBufferPipelineLayout = nullptr;
    }

    if (m_gBufferRenderPipeline)
    {
        wgpu.RenderPipelineRelease(m_gBufferRenderPipeline);
        m_gBufferRenderPipeline = nullptr;
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

void WGPUDeferredRenderingSample::createFloat16Texture()
{
    // Create the float16 texture.
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = WGPUTextureDimension_2D;
    textureDescriptor.size.width = m_width;
    textureDescriptor.size.height = m_height;
    textureDescriptor.size.depthOrArrayLayers = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.format = WGPUTextureFormat_RGBA16Float;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;

    m_float16Texture = wgpu.DeviceCreateTexture(m_device, &textureDescriptor);
    assert(m_float16Texture);
}

void WGPUDeferredRenderingSample::createAlbedoTexture()
{
    // Create the albedo texture.
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = WGPUTextureDimension_2D;
    textureDescriptor.size.width = m_width;
    textureDescriptor.size.height = m_height;
    textureDescriptor.size.depthOrArrayLayers = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.format = WGPUTextureFormat_BGRA8Unorm;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;

    m_albedoTexture = wgpu.DeviceCreateTexture(m_device, &textureDescriptor);
    assert(m_albedoTexture);
}

void WGPUDeferredRenderingSample::createDepthTexture()
{
    // Create the depth texture.
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = WGPUTextureDimension_2D;
    textureDescriptor.size.width = m_width;
    textureDescriptor.size.height = m_height;
    textureDescriptor.size.depthOrArrayLayers = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.format = WGPUTextureFormat_Depth24Plus;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;

    m_depthTexture = wgpu.DeviceCreateTexture(m_device, &textureDescriptor);
    assert(m_depthTexture);
}

void WGPUDeferredRenderingSample::createFloat16TextureView()
{
    WGPUTextureViewDescriptor textureViewDescriptor{};
    textureViewDescriptor.format = WGPUTextureFormat_RGBA16Float;
    textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    textureViewDescriptor.baseMipLevel = 0;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;

    m_float16TextureView = wgpu.TextureCreateView(m_float16Texture, &textureViewDescriptor);
    assert(m_float16TextureView);
}

void WGPUDeferredRenderingSample::createAlbedoTextureView()
{
    WGPUTextureViewDescriptor textureViewDescriptor{};
    textureViewDescriptor.format = WGPUTextureFormat_BGRA8Unorm;
    textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    textureViewDescriptor.baseMipLevel = 0;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;

    m_albedoTextureView = wgpu.TextureCreateView(m_albedoTexture, &textureViewDescriptor);
    assert(m_albedoTextureView);
}

void WGPUDeferredRenderingSample::createDepthTextureView()
{
    WGPUTextureViewDescriptor textureViewDescriptor{};
    textureViewDescriptor.format = WGPUTextureFormat_Depth24Plus;
    textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    textureViewDescriptor.baseMipLevel = 0;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;

    m_depthTextureView = wgpu.TextureCreateView(m_depthTexture, &textureViewDescriptor);
    assert(m_depthTextureView);
}

void WGPUDeferredRenderingSample::createGBufferBindGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 3> bindGroupLayoutEntries{
        WGPUBindGroupLayoutEntry{
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 1,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 2,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_Depth,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false,
            },
        }
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_gBufferBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_gBufferBindGroupLayout);
}

void WGPUDeferredRenderingSample::createGBufferBindGroup()
{
    std::array<WGPUBindGroupEntry, 3> bindGroupEntries{
        WGPUBindGroupEntry{
            .binding = 0,
            .textureView = m_float16TextureView,
        },
        WGPUBindGroupEntry{
            .binding = 1,
            .textureView = m_albedoTextureView,
        },
        WGPUBindGroupEntry{
            .binding = 2,
            .textureView = m_depthTextureView,
        }
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_gBufferBindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    m_gBufferBindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_gBufferBindGroup);
}

void WGPUDeferredRenderingSample::createGBufferPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_gBufferBindGroupLayout;

    m_gBufferPipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_gBufferPipelineLayout);
}

void WGPUDeferredRenderingSample::createGBufferRenderPipeline()
{
}

void WGPUDeferredRenderingSample::createShaderModules()
{
    {
        std::vector<char> fragmentDeferredRenderingShaderSource = utils::readFile(m_appDir / "fragmentDeferredRendering.wgsl", m_handle);

        std::string fragmentDeferredRenderingShaderCode(fragmentDeferredRenderingShaderSource.begin(), fragmentDeferredRenderingShaderSource.end());

        WGPUShaderModuleWGSLDescriptor fragmentDeferredRenderingShaderModuleWGSLDescriptor{};
        fragmentDeferredRenderingShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragmentDeferredRenderingShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = fragmentDeferredRenderingShaderCode.data(),
            .length = fragmentDeferredRenderingShaderCode.size(),
        };

        WGPUShaderModuleDescriptor fragmentDeferredRenderingShaderModuleDescriptor{};
        fragmentDeferredRenderingShaderModuleDescriptor.nextInChain = &fragmentDeferredRenderingShaderModuleWGSLDescriptor.chain;

        m_fragmentDeferredRenderingShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragmentDeferredRenderingShaderModuleDescriptor);
        assert(m_fragmentDeferredRenderingShaderModule);
    }

    {
        std::vector<char> fragmentGBufferDebugViewShaderSource = utils::readFile(m_appDir / "fragmentGBuffersDebugView.wgsl", m_handle);

        std::string fragmentGBufferDebugViewShaderCode(fragmentGBufferDebugViewShaderSource.begin(), fragmentGBufferDebugViewShaderSource.end());

        WGPUShaderModuleWGSLDescriptor fragmentGBufferDebugViewShaderModuleWGSLDescriptor{};
        fragmentGBufferDebugViewShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragmentGBufferDebugViewShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = fragmentGBufferDebugViewShaderCode.data(),
            .length = fragmentGBufferDebugViewShaderCode.size(),
        };

        WGPUShaderModuleDescriptor fragmentGBufferDebugViewShaderModuleDescriptor{};
        fragmentGBufferDebugViewShaderModuleDescriptor.nextInChain = &fragmentGBufferDebugViewShaderModuleWGSLDescriptor.chain;

        m_fragmentGBufferDebugViewShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragmentGBufferDebugViewShaderModuleDescriptor);
        assert(m_fragmentGBufferDebugViewShaderModule);
    }

    {
        std::vector<char> fragmentWriteGBuffersShaderSource = utils::readFile(m_appDir / "fragmentWriteGBuffers.wgsl", m_handle);

        std::string fragmentWriteGBuffersShaderCode(fragmentWriteGBuffersShaderSource.begin(), fragmentWriteGBuffersShaderSource.end());

        WGPUShaderModuleWGSLDescriptor fragmentWriteGBuffersShaderModuleWGSLDescriptor{};
        fragmentWriteGBuffersShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragmentWriteGBuffersShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = fragmentWriteGBuffersShaderCode.data(),
            .length = fragmentWriteGBuffersShaderCode.size(),
        };

        WGPUShaderModuleDescriptor fragmentWriteGBuffersShaderModuleDescriptor{};
        fragmentWriteGBuffersShaderModuleDescriptor.nextInChain = &fragmentWriteGBuffersShaderModuleWGSLDescriptor.chain;

        m_fragmentWriteGBuffersShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragmentWriteGBuffersShaderModuleDescriptor);
        assert(m_fragmentWriteGBuffersShaderModule);
    }

    {
        std::vector<char> vertexTextureQuadShaderSource = utils::readFile(m_appDir / "vertexTextureQuad.wgsl", m_handle);

        std::string vertexTextureQuadShaderCode(vertexTextureQuadShaderSource.begin(), vertexTextureQuadShaderSource.end());

        WGPUShaderModuleWGSLDescriptor vertexTextureQuadShaderModuleWGSLDescriptor{};
        vertexTextureQuadShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        vertexTextureQuadShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = vertexTextureQuadShaderCode.data(),
            .length = vertexTextureQuadShaderCode.size(),
        };

        WGPUShaderModuleDescriptor vertexTextureQuadShaderModuleDescriptor{};
        vertexTextureQuadShaderModuleDescriptor.nextInChain = &vertexTextureQuadShaderModuleWGSLDescriptor.chain;

        m_vertexTextureQuadShaderModule = wgpu.DeviceCreateShaderModule(m_device, &vertexTextureQuadShaderModuleDescriptor);
        assert(m_vertexTextureQuadShaderModule);
    }

    {
        std::vector<char> vertexWriteGBuffersShaderSource = utils::readFile(m_appDir / "vertexWriteGBuffers.wgsl", m_handle);

        std::string vertexWriteGBuffersShaderCode(vertexWriteGBuffersShaderSource.begin(), vertexWriteGBuffersShaderSource.end());

        WGPUShaderModuleWGSLDescriptor vertexWriteGBuffersShaderModuleWGSLDescriptor{};
        vertexWriteGBuffersShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        vertexWriteGBuffersShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = vertexWriteGBuffersShaderCode.data(),
            .length = vertexWriteGBuffersShaderCode.size(),
        };

        WGPUShaderModuleDescriptor vertexWriteGBuffersShaderModuleDescriptor{};
        vertexWriteGBuffersShaderModuleDescriptor.nextInChain = &vertexWriteGBuffersShaderModuleWGSLDescriptor.chain;

        m_vertexWriteGBuffersShaderModule = wgpu.DeviceCreateShaderModule(m_device, &vertexWriteGBuffersShaderModuleDescriptor);
        assert(m_vertexWriteGBuffersShaderModule);
    }

    {
        std::vector<char> lightUpdateShaderSource = utils::readFile(m_appDir / "lightUpdate.wgsl", m_handle);
        std::string lightUpdateShaderCode(lightUpdateShaderSource.begin(), lightUpdateShaderSource.end());

        WGPUShaderModuleWGSLDescriptor lightUpdateShaderModuleWGSLDescriptor{};
        lightUpdateShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        lightUpdateShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = lightUpdateShaderCode.data(),
            .length = lightUpdateShaderCode.size(),
        };

        WGPUShaderModuleDescriptor lightUpdateShaderModuleDescriptor{};
        lightUpdateShaderModuleDescriptor.nextInChain = &lightUpdateShaderModuleWGSLDescriptor.chain;

        m_lightUpdateShaderModule = wgpu.DeviceCreateShaderModule(m_device, &lightUpdateShaderModuleDescriptor);
        assert(m_lightUpdateShaderModule);
    }
    // m_fragmentDeferredRenderingShaderModule = createShaderModule("shaders/deferred_rendering.frag.spv");
    // m_fragmentGBufferDebugViewShaderModule = createShaderModule("shaders/gbuffer_debug_view.frag.spv");
    // m_fragmentWriteGBuffersShaderModule = createShaderModule("shaders/write_gbuffers.frag.spv");
    // m_vertexTextureQuadShaderModule = createShaderModule("shaders/texture_quad.vert.spv");
    // m_vertexWriteGBuffersShaderModule = createShaderModule("shaders/write_gbuffers.vert.spv");
}

} // namespace jipu
