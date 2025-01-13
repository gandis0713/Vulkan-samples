#include "wgpu_triangle.h"

#include "file.h"
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUTriangleSample::WGPUTriangleSample(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
    m_imgui.emplace(this);
}

WGPUTriangleSample::~WGPUTriangleSample()
{
    finalizeContext();
}

void WGPUTriangleSample::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);
}

void WGPUTriangleSample::onUpdate()
{
    WGPUSample::onUpdate();
}

void WGPUTriangleSample::onDraw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
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

void WGPUTriangleSample::initializeContext()
{
    WGPUSample::initializeContext();

    createShaderModule();
    createPipelineLayout();
    createPipeline();
}

void WGPUTriangleSample::finalizeContext()
{
    // TODO: check ways release and destory.
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

    if (m_vertSPIRVShaderModule)
    {
        wgpu.ShaderModuleRelease(m_vertSPIRVShaderModule);
        m_vertSPIRVShaderModule = nullptr;
    }

    if (m_fragSPIRVShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragSPIRVShaderModule);
        m_fragSPIRVShaderModule = nullptr;
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

void WGPUTriangleSample::createShaderModule()
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

void WGPUTriangleSample::createPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_pipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_pipelineLayout);
}

void WGPUTriangleSample::createPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_Back;
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
    multisampleState.count = 1;
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

} // namespace jipu