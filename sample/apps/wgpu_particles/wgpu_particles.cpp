#include "wgpu_particles.h"

#include "file.h"
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUParticlesSample::WGPUParticlesSample(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
    m_imgui.emplace(this);
}

WGPUParticlesSample::~WGPUParticlesSample()
{
    finalizeContext();
}

void WGPUParticlesSample::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);
}

void WGPUParticlesSample::onUpdate()
{
    WGPUSample::onUpdate();
}

void WGPUParticlesSample::onDraw()
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

void WGPUParticlesSample::initializeContext()
{
    WGPUSample::initializeContext();

    createShaderModule();
    createRenderPipelineLayout();
    createRenderPipeline();
    createComputePipelineLayout();
    createComputePipeline();
}

void WGPUParticlesSample::finalizeContext()
{
    // TODO: check ways release and destory.

    if (m_computePipeline)
    {
        wgpu.ComputePipelineRelease(m_computePipeline);
        m_computePipeline = nullptr;
    }

    if (m_computePipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_computePipelineLayout);
        m_computePipelineLayout = nullptr;
    }

    if (m_renderPipeline)
    {
        wgpu.RenderPipelineRelease(m_renderPipeline);
        m_renderPipeline = nullptr;
    }

    if (m_renderPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_renderPipelineLayout);
        m_renderPipelineLayout = nullptr;
    }

    if (m_wgslParticleShaderModule)
    {
        wgpu.ShaderModuleRelease(m_wgslParticleShaderModule);
        m_wgslParticleShaderModule = nullptr;
    }

    if (m_wgslProbablilityMapShaderModule)
    {
        wgpu.ShaderModuleRelease(m_wgslProbablilityMapShaderModule);
        m_wgslProbablilityMapShaderModule = nullptr;
    }

    WGPUSample::finalizeContext();
}

void WGPUParticlesSample::createShaderModule()
{
    std::vector<char> particleShaderSource = utils::readFile(m_appDir / "particle.wgsl", m_handle);
    std::vector<char> probablilityShaderSource = utils::readFile(m_appDir / "probabilityMap.wgsl", m_handle);

    std::string particleShaderCode(particleShaderSource.begin(), particleShaderSource.end());
    std::string probablilityShaderCode(probablilityShaderSource.begin(), probablilityShaderSource.end());

    WGPUShaderModuleWGSLDescriptor particleShaderModuleWGSLDescriptor{};
    particleShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    particleShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = particleShaderCode.data(), .length = particleShaderCode.size() };

    WGPUShaderModuleDescriptor particleShaderModuleDescriptor{};
    particleShaderModuleDescriptor.nextInChain = &particleShaderModuleWGSLDescriptor.chain;

    m_wgslParticleShaderModule = wgpu.DeviceCreateShaderModule(m_device, &particleShaderModuleDescriptor);

    assert(m_wgslParticleShaderModule);

    WGPUShaderModuleWGSLDescriptor probablilityShaderModuleWGSLDescriptor{};
    probablilityShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    probablilityShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = probablilityShaderCode.data(), .length = probablilityShaderCode.size() };

    WGPUShaderModuleDescriptor probablilityShaderModuleDescriptor{};
    probablilityShaderModuleDescriptor.nextInChain = &probablilityShaderModuleWGSLDescriptor.chain;

    m_wgslProbablilityMapShaderModule = wgpu.DeviceCreateShaderModule(m_device, &probablilityShaderModuleDescriptor);

    assert(m_wgslProbablilityMapShaderModule);
}

void WGPUParticlesSample::createRenderPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_renderPipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_renderPipelineLayout);
}

void WGPUParticlesSample::createRenderPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_Back;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    std::string entryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    vertexState.module = m_wgslParticleShaderModule;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceConfigure.format;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    fragState.module = m_wgslProbablilityMapShaderModule;
    fragState.targetCount = 1;
    fragState.targets = &colorTargetState;

    // WGPUDepthStencilState depthStencilState{};
    // depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_renderPipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.multisample = multisampleState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragState;

    m_renderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);

    assert(m_renderPipeline);
}

void WGPUParticlesSample::createComputePipeline()
{
    WGPUComputePipelineDescriptor computePipelineDescriptor{};
    computePipelineDescriptor.layout = m_computePipelineLayout;
    computePipelineDescriptor.compute.entryPoint = WGPUStringView{ .data = "main", .length = 4 };
    computePipelineDescriptor.compute.module = m_wgslParticleShaderModule;

    m_computePipeline = wgpu.DeviceCreateComputePipeline(m_device, &computePipelineDescriptor);

    assert(m_computePipeline);
}

void WGPUParticlesSample::createComputePipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_computePipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_computePipelineLayout);
}

} // namespace jipu