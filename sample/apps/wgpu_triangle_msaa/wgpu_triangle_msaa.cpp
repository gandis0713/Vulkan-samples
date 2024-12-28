#include "wgpu_triangle_msaa.h"

#include "file.h"
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUTriangleSampleMSAA::WGPUTriangleSampleMSAA(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
}

WGPUTriangleSampleMSAA::~WGPUTriangleSampleMSAA()
{
    finalizeContext();
}

void WGPUTriangleSampleMSAA::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);
    // changeAPI(APIType::kDawn);
}

void WGPUTriangleSampleMSAA::onUpdate()
{
    WGPUSample::onUpdate();
}

void WGPUTriangleSampleMSAA::onResize(uint32_t width, uint32_t height)
{
    WGPUSample::onResize(width, height);

    releaseRenderTextureView();
    releaseRenderTexture();

    createRenderTexture();
    createRenderTextureView();
}

void WGPUTriangleSampleMSAA::onDraw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = m_renderTextureView;
    colorAttachment.resolveTarget = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

    WGPURenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;

    WGPURenderPassEncoder renderPassEncoder = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpu.RenderPassEncoderSetPipeline(renderPassEncoder, m_renderPipeline);
    wgpu.RenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);
    wgpu.RenderPassEncoderEnd(renderPassEncoder);
    wgpu.RenderPassEncoderRelease(renderPassEncoder);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu.QueueSubmit(m_queue, 1, &commandBuffer);
    wgpu.SurfacePresent(m_surface);

    wgpu.CommandBufferRelease(commandBuffer);
    wgpu.CommandEncoderRelease(commandEncoder);
    wgpu.TextureViewRelease(surfaceTextureView);
    wgpu.TextureRelease(surfaceTexture.texture);
}

void WGPUTriangleSampleMSAA::initializeContext()
{
    WGPUSample::initializeContext();

    createRenderTexture();
    createRenderTextureView();
    createShaderModule();
    createRenderPipelineLayout();
    createRenderPipeline();
}

void WGPUTriangleSampleMSAA::finalizeContext()
{
    // TODO: check ways release and destory.

    releaseRenderTextureView();
    releaseRenderTexture();

    if (m_renderPipeline)
    {
        wgpu.RenderPipelineRelease(m_renderPipeline);
        m_renderPipeline = nullptr;
    }

    if (m_pipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_pipelineLayout);
        m_pipelineLayout = nullptr;
    }

    if (m_vertWGSLShaderModule)
    {
        wgpu.ShaderModuleRelease(m_vertWGSLShaderModule);
        m_vertWGSLShaderModule = nullptr;
    }

    if (m_fragWGSLShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragWGSLShaderModule);
        m_fragWGSLShaderModule = nullptr;
    }

    WGPUSample::finalizeContext();
}

void WGPUTriangleSampleMSAA::createRenderTexture()
{
    if (m_renderTexture)
    {
        wgpu.TextureRelease(m_renderTexture);
        m_renderTexture = nullptr;
    }

    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size.width = m_width;
    descriptor.size.height = m_height;
    descriptor.size.depthOrArrayLayers = 1;
    descriptor.sampleCount = m_sampleCount;
    descriptor.format = m_surfaceConfigure.format;
    descriptor.mipLevelCount = 1;
    descriptor.usage = WGPUTextureUsage_RenderAttachment;

    m_renderTexture = wgpu.DeviceCreateTexture(m_device, &descriptor);
    assert(m_renderTexture);
}

void WGPUTriangleSampleMSAA::createRenderTextureView()
{
    if (m_renderTextureView)
    {
        wgpu.TextureViewRelease(m_renderTextureView);
        m_renderTextureView = nullptr;
    }

    WGPUTextureViewDescriptor descriptor{};
    descriptor.format = m_surfaceConfigure.format;
    descriptor.dimension = WGPUTextureViewDimension_2D;
    descriptor.aspect = WGPUTextureAspect_All;
    descriptor.usage = WGPUTextureUsage_RenderAttachment;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = 1;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;

    m_renderTextureView = wgpu.TextureCreateView(m_renderTexture, &descriptor);
}

void WGPUTriangleSampleMSAA::createShaderModule()
{
    std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "triangle.vert.wgsl", m_handle);
    std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "triangle.frag.wgsl", m_handle);

    std::string vertexShaderCode(vertexShaderSource.begin(), vertexShaderSource.end());
    std::string fragmentShaderCode(fragmentShaderSource.begin(), fragmentShaderSource.end());

    WGPUShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
    vertexShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    vertexShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = vertexShaderCode.data(), .length = vertexShaderCode.size() };

    WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
    vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleWGSLDescriptor.chain;

    m_vertWGSLShaderModule = wgpu.DeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

    assert(m_vertWGSLShaderModule);

    WGPUShaderModuleWGSLDescriptor fragShaderModuleWGSLDescriptor{};
    fragShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    fragShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = fragmentShaderCode.data(), .length = fragmentShaderCode.size() };

    WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
    fragShaderModuleDescriptor.nextInChain = &fragShaderModuleWGSLDescriptor.chain;

    m_fragWGSLShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);

    assert(m_fragWGSLShaderModule);
}

void WGPUTriangleSampleMSAA::createRenderPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_pipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_pipelineLayout);
}

void WGPUTriangleSampleMSAA::createRenderPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_None; // TODO: backface culling
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    std::string entryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    vertexState.module = m_vertWGSLShaderModule;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceConfigure.format;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    fragState.module = m_fragWGSLShaderModule;

    fragState.targetCount = 1;
    fragState.targets = &colorTargetState;

    // WGPUDepthStencilState depthStencilState{};
    // depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;

    WGPUMultisampleState multisampleState{};
    multisampleState.count = m_sampleCount;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_pipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.multisample = multisampleState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragState;

    m_renderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);

    assert(m_renderPipeline);
}

void WGPUTriangleSampleMSAA::releaseRenderTexture()
{
    if (m_renderTexture)
    {
        wgpu.TextureRelease(m_renderTexture);
        m_renderTexture = nullptr;
    }
}

void WGPUTriangleSampleMSAA::releaseRenderTextureView()
{
    if (m_renderTextureView)
    {
        wgpu.TextureViewRelease(m_renderTextureView);
        m_renderTextureView = nullptr;
    }
}

} // namespace jipu