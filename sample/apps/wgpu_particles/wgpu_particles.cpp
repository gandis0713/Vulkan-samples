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

    changeAPI(APIType::kDawn);
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

    createParticleBuffer();
    createUniformBuffer();
    createDepthTexture();
    createParticleShaderModule();
    createProbabilityMapShaderModule();
    createProbabilityMapImportLevelPipelineLayout();
    createProbabilityMapImportLevelPipeline();
    createProbabilityMapExportLevelPipelineLayout();
    createProbabilityMapExportLevelPipeline();
    createComputePipelineLayout();
    createComputePipeline();
    createRenderPipelineLayout();
    createRenderPipeline();
}

void WGPUParticlesSample::finalizeContext()
{
    // TODO: check ways release and destory.

    if (m_particleBuffer)
    {
        wgpu.BufferRelease(m_particleBuffer);
        m_particleBuffer = nullptr;
    }

    if (m_uniformBuffer)
    {
        wgpu.BufferRelease(m_uniformBuffer);
        m_uniformBuffer = nullptr;
    }

    if (m_depthTexture)
    {
        wgpu.TextureRelease(m_depthTexture);
        m_depthTexture = nullptr;
    }

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

    if (m_probabilityMapExportLevelPipeline)
    {
        wgpu.ComputePipelineRelease(m_probabilityMapExportLevelPipeline);
        m_probabilityMapExportLevelPipeline = nullptr;
    }

    if (m_probabilityMapExportLevelPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_probabilityMapExportLevelPipelineLayout);
        m_probabilityMapExportLevelPipelineLayout = nullptr;
    }

    if (m_probabilityMapImportLevelPipeline)
    {
        wgpu.ComputePipelineRelease(m_probabilityMapImportLevelPipeline);
        m_probabilityMapImportLevelPipeline = nullptr;
    }

    if (m_probabilityMapImportLevelPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_probabilityMapImportLevelPipelineLayout);
        m_probabilityMapImportLevelPipelineLayout = nullptr;
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

void WGPUParticlesSample::createParticleBuffer()
{
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = m_numParticles * m_particleInstanceByteSize;
    bufferDescriptor.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_Storage;
    bufferDescriptor.mappedAtCreation = false;

    m_particleBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);

    assert(m_particleBuffer);
}

void WGPUParticlesSample::createUniformBuffer()
{
    const uint32_t uniformBufferSize =
        4 * 4 * 4 + // modelViewProjectionMatrix : mat4x4f
        3 * 4 +     // right : vec3f
        4 +         // padding
        3 * 4 +     // up : vec3f
        4 +         // padding
        0;

    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = uniformBufferSize;
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_uniformBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);

    assert(m_uniformBuffer);
}

void WGPUParticlesSample::createDepthTexture()
{
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = WGPUTextureDimension_2D;
    textureDescriptor.size.width = m_width;
    textureDescriptor.size.height = m_height;
    textureDescriptor.size.depthOrArrayLayers = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.format = WGPUTextureFormat_Depth24Plus;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = WGPUTextureUsage_RenderAttachment;

    m_depthTexture = wgpu.DeviceCreateTexture(m_device, &textureDescriptor);

    assert(m_depthTexture);
}

void WGPUParticlesSample::createParticleShaderModule()
{
    std::vector<char> particleShaderSource = utils::readFile(m_appDir / "particle.wgsl", m_handle);

    std::string particleShaderCode(particleShaderSource.begin(), particleShaderSource.end());
    std::string probablilityShaderCode(probablilityShaderSource.begin(), probablilityShaderSource.end());

    WGPUShaderModuleWGSLDescriptor particleShaderModuleWGSLDescriptor{};
    particleShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    particleShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = particleShaderCode.data(), .length = particleShaderCode.size() };

    WGPUShaderModuleDescriptor particleShaderModuleDescriptor{};
    particleShaderModuleDescriptor.nextInChain = &particleShaderModuleWGSLDescriptor.chain;

    m_wgslParticleShaderModule = wgpu.DeviceCreateShaderModule(m_device, &particleShaderModuleDescriptor);

    assert(m_wgslParticleShaderModule);
}

void WGPUParticlesSample::createProbabilityMapShaderModule()
{
    std::vector<char> probablilityShaderSource = utils::readFile(m_appDir / "probabilityMap.wgsl", m_handle);

    WGPUShaderModuleWGSLDescriptor probablilityShaderModuleWGSLDescriptor{};
    probablilityShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    probablilityShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = probablilityShaderCode.data(), .length = probablilityShaderCode.size() };

    WGPUShaderModuleDescriptor probablilityShaderModuleDescriptor{};
    probablilityShaderModuleDescriptor.nextInChain = &probablilityShaderModuleWGSLDescriptor.chain;

    m_wgslProbablilityMapShaderModule = wgpu.DeviceCreateShaderModule(m_device, &probablilityShaderModuleDescriptor);

    assert(m_wgslProbablilityMapShaderModule);
}

void WGPUParticlesSample::createProbabilityMapImportLevelPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_probabilityMapImportLevelPipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_probabilityMapImportLevelPipelineLayout);
}

void WGPUParticlesSample::createProbabilityMapImportLevelPipeline()
{
    std::string entryPoint = "import_level";
    WGPUComputePipelineDescriptor computePipelineDescriptor{};
    computePipelineDescriptor.layout = m_probabilityMapImportLevelPipelineLayout;
    computePipelineDescriptor.compute.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    computePipelineDescriptor.compute.module = m_wgslProbablilityMapShaderModule;

    m_probabilityMapImportLevelPipeline = wgpu.DeviceCreateComputePipeline(m_device, &computePipelineDescriptor);

    assert(m_probabilityMapImportLevelPipeline);
}

void WGPUParticlesSample::createProbabilityMapExportLevelPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_probabilityMapExportLevelPipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_probabilityMapExportLevelPipelineLayout);
}

void WGPUParticlesSample::createProbabilityMapExportLevelPipeline()
{
    std::string entryPoint = "export_level";
    WGPUComputePipelineDescriptor computePipelineDescriptor{};
    computePipelineDescriptor.layout = m_probabilityMapExportLevelPipelineLayout;
    computePipelineDescriptor.compute.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    computePipelineDescriptor.compute.module = m_wgslProbablilityMapShaderModule;

    m_probabilityMapExportLevelPipeline = wgpu.DeviceCreateComputePipeline(m_device, &computePipelineDescriptor);

    assert(m_probabilityMapExportLevelPipeline);
}

void WGPUParticlesSample::createComputePipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_computePipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_computePipelineLayout);
}

void WGPUParticlesSample::createComputePipeline()
{
    std::string entryPoint = "simulate";
    WGPUComputePipelineDescriptor computePipelineDescriptor{};
    computePipelineDescriptor.layout = m_computePipelineLayout;
    computePipelineDescriptor.compute.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    computePipelineDescriptor.compute.module = m_wgslParticleShaderModule;

    m_computePipeline = wgpu.DeviceCreateComputePipeline(m_device, &computePipelineDescriptor);

    assert(m_computePipeline);
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

    std::string vertexEntryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = vertexEntryPoint.data(), .length = vertexEntryPoint.size() };
    vertexState.module = m_wgslParticleShaderModule;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceConfigure.format;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    std::string fragEntryPoint = "main";
    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = fragEntryPoint.data(), .length = fragEntryPoint.size() };
    fragState.module = m_wgslParticleShaderModule;
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

} // namespace jipu