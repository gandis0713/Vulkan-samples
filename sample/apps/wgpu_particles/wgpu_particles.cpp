#include "wgpu_particles.h"

#include "file.h"
#include "image.h"

#include <cmath>
#include <random>
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

    {
        SimulationUBO ubo;
        ubo.simulateDeltaTime = m_simulationParams.simulate ? m_simulationParams.deltaTime : 0.0f;
        ubo.brightnessFactor = m_simulationParams.brightnessFactor;
        ubo.padding1 = 0.0f;
        ubo.padding2 = 0.0f;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis100(0.0f, 100.0f);
        std::uniform_real_distribution<float> dis1(0.0f, 1.0f);

        ubo.seedX = dis100(gen);
        ubo.seedY = dis100(gen);
        ubo.seedZ = 1.0f + dis1(gen);
        ubo.seedW = 1.0f + dis1(gen);

        wgpu.QueueWriteBuffer(m_queue, m_simulationUBOBuffer, 0, &ubo, sizeof(SimulationUBO));
    }
    {
        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height); // 종횡비
        float fov = (2.0f * glm::pi<float>()) / 5.0f;                              // (2 * PI) / 5 라디안
        float nearPlane = 1.0f;
        float farPlane = 100.0f;
        glm::mat4 projection = glm::perspective(fov, aspect, nearPlane, farPlane);

        glm::mat4 view = glm::mat4(1.0f);                                                // 단위 행렬로 초기화
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));                       // (0, 0, -3)으로 이동
        view = glm::rotate(view, -0.2f * glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)); // X축을 기준으로 -0.2 * PI 라디안 회전

        // 5. 프로젝션 행렬과 뷰 행렬을 곱하여 MVP 행렬 생성
        glm::mat4 mvp = projection * view;

        MatrixUBO ubo;
        ubo.mvp = mvp;
        ubo.right = glm::vec3(view[0][0], view[1][0], view[2][0]);
        ubo.padding1 = 0.0f;
        ubo.up = glm::vec3(view[0][1], view[1][1], view[2][1]);
        ubo.padding2 = 0.0f;

        wgpu.QueueWriteBuffer(m_queue, m_uniformBuffer, 0, &ubo, sizeof(MatrixUBO));
    }
}

void WGPUParticlesSample::onDraw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);
    WGPUTextureView depthTextureView = wgpu.TextureCreateView(m_depthTexture, NULL);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

    WGPURenderPassDepthStencilAttachment depthStencilAttachment{};
    depthStencilAttachment.view = depthTextureView;
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthClearValue = 1.0f;

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPUComputePassDescriptor computePassDescriptor{};
    WGPUComputePassEncoder computePassEncoder = wgpu.CommandEncoderBeginComputePass(commandEncoder, &computePassDescriptor);

    wgpu.ComputePassEncoderSetPipeline(computePassEncoder, m_computePipeline);
    wgpu.ComputePassEncoderSetBindGroup(computePassEncoder, 0, m_computeBindGroup, 0, nullptr);
    wgpu.ComputePassEncoderDispatchWorkgroups(computePassEncoder, std::ceil(m_numParticles / 64), 1, 1);
    wgpu.ComputePassEncoderEnd(computePassEncoder);

    WGPURenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;
    renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;

    WGPURenderPassEncoder renderPassEncoder = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
    wgpu.RenderPassEncoderSetPipeline(renderPassEncoder, m_renderPipeline);
    wgpu.RenderPassEncoderSetBindGroup(renderPassEncoder, 0, m_uniformBindGroup, 0, nullptr);
    wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_particleBuffer, 0, m_numParticles * m_particleInstanceByteSize);
    wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 1, m_quadBuffer, 0, 6 * 2 * 4);
    wgpu.RenderPassEncoderDraw(renderPassEncoder, 6, m_numParticles, 0, 0);
    wgpu.RenderPassEncoderEnd(renderPassEncoder);

    drawImGui(commandEncoder, surfaceTextureView);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu.QueueSubmit(m_queue, 1, &commandBuffer);
    wgpu.SurfacePresent(m_surface);

    wgpu.CommandBufferRelease(commandBuffer);
    wgpu.CommandEncoderRelease(commandEncoder);
    wgpu.TextureViewRelease(surfaceTextureView);
    wgpu.TextureViewRelease(depthTextureView);
    wgpu.TextureRelease(surfaceTexture.texture);
}

void WGPUParticlesSample::initializeContext()
{
    WGPUSample::initializeContext();

    createQuadBuffer();
    createParticleBuffer();
    createUniformBuffer();
    createSimulationUBOBuffer();
    createImageTexture();
    createImageTextureView();
    createDepthTexture();
    createParticleShaderModule();
    createProbabilityMapShaderModule();
    createProbabilityMapUBOBuffer();
    createProbabilityMapBufferA();
    createProbabilityMapBufferB();
    createProbabilityMapImportLevelBindGroupLayout();
    createProbabilityMapExportLevelBindGroupLayout();
    createProbabilityMapImportLevelPipelineLayout();
    createProbabilityMapImportLevelPipeline();
    createProbabilityMapExportLevelPipelineLayout();
    createProbabilityMapExportLevelPipeline();
    generateProbabilityMap();
    createComputeBindGroupLayout();
    createComputeBindGroup();
    createUniformBindGroupLayout();
    createUniformBindGroup();
    createComputePipelineLayout();
    createComputePipeline();
    createRenderPipelineLayout();
    createRenderPipeline();
}

void WGPUParticlesSample::finalizeContext()
{
    // TODO: check ways release and destory.

    if (m_quadBuffer)
    {
        wgpu.BufferRelease(m_quadBuffer);
        m_quadBuffer = nullptr;
    }

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

    if (m_simulationUBOBuffer)
    {
        wgpu.BufferRelease(m_simulationUBOBuffer);
        m_simulationUBOBuffer = nullptr;
    }

    if (m_imageTexture)
    {
        wgpu.TextureRelease(m_imageTexture);
        m_imageTexture = nullptr;
    }

    if (m_imageTextureView)
    {
        wgpu.TextureViewRelease(m_imageTextureView);
        m_imageTextureView = nullptr;
    }

    if (m_depthTexture)
    {
        wgpu.TextureRelease(m_depthTexture);
        m_depthTexture = nullptr;
    }

    if (m_probabilityMapUBOBuffer)
    {
        wgpu.BufferRelease(m_probabilityMapUBOBuffer);
        m_probabilityMapUBOBuffer = nullptr;
    }

    if (m_probabilityMapBufferA)
    {
        wgpu.BufferRelease(m_probabilityMapBufferA);
        m_probabilityMapBufferA = nullptr;
    }

    if (m_probabilityMapBufferB)
    {
        wgpu.BufferRelease(m_probabilityMapBufferB);
        m_probabilityMapBufferB = nullptr;
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

    if (m_probabilityMapImportLevelBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_probabilityMapImportLevelBindGroupLayout);
        m_probabilityMapImportLevelBindGroupLayout = nullptr;
    }

    if (m_probabilityMapExportLevelBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_probabilityMapExportLevelBindGroupLayout);
        m_probabilityMapExportLevelBindGroupLayout = nullptr;
    }

    if (m_probabilityMapExportLevelPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_probabilityMapExportLevelPipelineLayout);
        m_probabilityMapExportLevelPipelineLayout = nullptr;
    }

    if (m_probabilityMapExportLevelPipeline)
    {
        wgpu.ComputePipelineRelease(m_probabilityMapExportLevelPipeline);
        m_probabilityMapExportLevelPipeline = nullptr;
    }

    if (m_probabilityMapImportLevelPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_probabilityMapImportLevelPipelineLayout);
        m_probabilityMapImportLevelPipelineLayout = nullptr;
    }

    if (m_probabilityMapImportLevelPipeline)
    {
        wgpu.ComputePipelineRelease(m_probabilityMapImportLevelPipeline);
        m_probabilityMapImportLevelPipeline = nullptr;
    }

    if (m_computeBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_computeBindGroupLayout);
        m_computeBindGroupLayout = nullptr;
    }

    if (m_computeBindGroup)
    {
        wgpu.BindGroupRelease(m_computeBindGroup);
        m_computeBindGroup = nullptr;
    }

    if (m_uniformBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_uniformBindGroupLayout);
        m_uniformBindGroupLayout = nullptr;
    }

    if (m_uniformBindGroup)
    {
        wgpu.BindGroupRelease(m_uniformBindGroup);
        m_uniformBindGroup = nullptr;
    }

    if (m_computePipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_computePipelineLayout);
        m_computePipelineLayout = nullptr;
    }

    if (m_computePipeline)
    {
        wgpu.ComputePipelineRelease(m_computePipeline);
        m_computePipeline = nullptr;
    }

    if (m_renderPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_renderPipelineLayout);
        m_renderPipelineLayout = nullptr;
    }

    if (m_renderPipeline)
    {
        wgpu.RenderPipelineRelease(m_renderPipeline);
        m_renderPipeline = nullptr;
    }

    WGPUSample::finalizeContext();
}

void WGPUParticlesSample::createQuadBuffer()
{
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = 6 * 2 * 4;
    bufferDescriptor.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_quadBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);

    assert(m_quadBuffer);

    // prettier-ignore
    float vertexData[12] = {
        -1.0,
        -1.0,
        +1.0,
        -1.0,
        -1.0,
        +1.0,
        -1.0,
        +1.0,
        +1.0,
        -1.0,
        +1.0,
        +1.0,
    };

    // void* mappedVertexPtr = wgpu.BufferGetMappedRange(vertexBuffer, 0, vertexBufferSize);
    // memcpy(mappedVertexPtr, sphereMesh.vertices.data(), vertexBufferSize);
    // wgpu.BufferUnmap(vertexBuffer);

    wgpu.QueueWriteBuffer(m_queue, m_quadBuffer, 0, &vertexData, bufferDescriptor.size);
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
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = m_uniformBufferSize;
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_uniformBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);

    assert(m_uniformBuffer);
}

void WGPUParticlesSample::createSimulationUBOBuffer()
{
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = m_simulationUBOBufferSize;
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_simulationUBOBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    assert(m_simulationUBOBuffer);
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

void WGPUParticlesSample::createImageTexture()
{
    std::vector<char> buffer = utils::readFile(m_appDir / "webgpu.png", m_handle);
    auto image = std::make_unique<Image>(buffer.data(), buffer.size());

    unsigned char* pixels = static_cast<unsigned char*>(image->getPixels());
    uint32_t width = image->getWidth();
    uint32_t height = image->getHeight();
    uint32_t channel = image->getChannel();
    uint64_t imageSize = sizeof(unsigned char) * width * height * channel;
    // uint32_t mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    // if (mipLevelCount > 10)
    //     mipLevelCount = 10;

    while (m_textureWidth < width || m_textureHeight < height)
    {
        m_textureWidth *= 2;
        m_textureHeight *= 2;
        m_numMipLevels++;
    }

    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size.width = width;
    descriptor.size.height = height;
    descriptor.size.depthOrArrayLayers = 1;
    descriptor.sampleCount = 1;
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.mipLevelCount = m_numMipLevels;
    descriptor.usage = WGPUTextureUsage_TextureBinding |
                       WGPUTextureUsage_CopyDst |
                       WGPUTextureUsage_RenderAttachment |
                       WGPUTextureUsage_StorageBinding;

    m_imageTexture = wgpu.DeviceCreateTexture(m_device, &descriptor);
    assert(m_imageTexture);

    WGPUImageCopyTexture imageCopyTexture{};
    imageCopyTexture.texture = m_imageTexture;
    imageCopyTexture.mipLevel = 0;
    imageCopyTexture.origin = { .x = 0, .y = 0, .z = 0 };
    imageCopyTexture.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout dataLayout{};
    dataLayout.offset = 0;
    dataLayout.bytesPerRow = sizeof(unsigned char) * width * channel;
#if defined(USE_DAWN_HEADER)
    dataLayout.rowsPerImage = height;
#else
    dataLayout.rowsPerTexture = height;
#endif

    wgpu.QueueWriteTexture(m_queue, &imageCopyTexture, pixels, imageSize, &dataLayout, &descriptor.size);
}

void WGPUParticlesSample::createImageTextureView()
{
    WGPUTextureViewDescriptor descriptor{};
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.dimension = WGPUTextureViewDimension_2D;
    descriptor.aspect = WGPUTextureAspect_All;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = m_numMipLevels;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;

    m_imageTextureView = wgpu.TextureCreateView(m_imageTexture, &descriptor);
    assert(m_imageTextureView);
}

void WGPUParticlesSample::createParticleShaderModule()
{
    std::vector<char> particleShaderSource = utils::readFile(m_appDir / "particle.wgsl", m_handle);

    std::string particleShaderCode(particleShaderSource.begin(), particleShaderSource.end());

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
    std::string probablilityShaderCode(probablilityShaderSource.begin(), probablilityShaderSource.end());

    WGPUShaderModuleWGSLDescriptor probablilityShaderModuleWGSLDescriptor{};
    probablilityShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    probablilityShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = probablilityShaderCode.data(), .length = probablilityShaderCode.size() };

    WGPUShaderModuleDescriptor probablilityShaderModuleDescriptor{};
    probablilityShaderModuleDescriptor.nextInChain = &probablilityShaderModuleWGSLDescriptor.chain;

    m_wgslProbablilityMapShaderModule = wgpu.DeviceCreateShaderModule(m_device, &probablilityShaderModuleDescriptor);

    assert(m_wgslProbablilityMapShaderModule);
}

void WGPUParticlesSample::createProbabilityMapUBOBuffer()
{
    uint32_t probabilityMapUBOBufferSize =
        1 * 4 + // stride
        3 * 4 + // padding
        0;

    WGPUBufferDescriptor probabilityMapUBOBufferDescriptor{};
    probabilityMapUBOBufferDescriptor.size = probabilityMapUBOBufferSize;
    probabilityMapUBOBufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    probabilityMapUBOBufferDescriptor.mappedAtCreation = false;

    m_probabilityMapUBOBuffer = wgpu.DeviceCreateBuffer(m_device, &probabilityMapUBOBufferDescriptor);
    assert(m_probabilityMapUBOBuffer);

    wgpu.QueueWriteBuffer(m_queue, m_probabilityMapUBOBuffer, 0, &m_textureWidth, sizeof(uint32_t));
}

void WGPUParticlesSample::createProbabilityMapBufferA()
{
    WGPUBufferDescriptor probabilityMapBufferDescriptor{};
    probabilityMapBufferDescriptor.size = m_textureWidth * m_textureHeight * 4;
    probabilityMapBufferDescriptor.usage = WGPUBufferUsage_Storage;
    probabilityMapBufferDescriptor.mappedAtCreation = false;

    m_probabilityMapBufferA = wgpu.DeviceCreateBuffer(m_device, &probabilityMapBufferDescriptor);
    assert(m_probabilityMapBufferA);
}

void WGPUParticlesSample::createProbabilityMapBufferB()
{
    WGPUBufferDescriptor probabilityMapBufferDescriptor{};
    probabilityMapBufferDescriptor.size = m_textureWidth * m_textureHeight * 4;
    probabilityMapBufferDescriptor.usage = WGPUBufferUsage_Storage;
    probabilityMapBufferDescriptor.mappedAtCreation = false;

    m_probabilityMapBufferB = wgpu.DeviceCreateBuffer(m_device, &probabilityMapBufferDescriptor);
    assert(m_probabilityMapBufferB);
}

void WGPUParticlesSample::createProbabilityMapImportLevelBindGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 4> bindGroupLayoutEntries = {
        WGPUBindGroupLayoutEntry{ .binding = 0,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_Uniform } },
        WGPUBindGroupLayoutEntry{ .binding = 1,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_ReadOnlyStorage } },
        WGPUBindGroupLayoutEntry{ .binding = 2,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_Storage } },
        WGPUBindGroupLayoutEntry{ .binding = 3,
                                  .visibility = WGPUShaderStage_Compute,
                                  .texture = { .sampleType = WGPUTextureSampleType_Float,
                                               .viewDimension = WGPUTextureViewDimension_2D,
                                               .multisampled = WGPUOptionalBool_False } },
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_probabilityMapImportLevelBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_probabilityMapImportLevelBindGroupLayout);
}

void WGPUParticlesSample::createProbabilityMapExportLevelBindGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 4> bindGroupLayoutEntries = {
        WGPUBindGroupLayoutEntry{ .binding = 0,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_Uniform } },
        WGPUBindGroupLayoutEntry{ .binding = 1,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_ReadOnlyStorage } },
        WGPUBindGroupLayoutEntry{ .binding = 2,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_Storage } },
        WGPUBindGroupLayoutEntry{ .binding = 3,
                                  .visibility = WGPUShaderStage_Compute,
                                  .storageTexture = { .access = WGPUStorageTextureAccess_WriteOnly,
                                                      .format = WGPUTextureFormat_RGBA8Unorm,
                                                      .viewDimension = WGPUTextureViewDimension_2D } },
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_probabilityMapExportLevelBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_probabilityMapExportLevelBindGroupLayout);
}

void WGPUParticlesSample::createProbabilityMapImportLevelPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_probabilityMapImportLevelBindGroupLayout;

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
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_probabilityMapExportLevelBindGroupLayout;

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

void WGPUParticlesSample::generateProbabilityMap()
{
    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    std::vector<WGPUBindGroup> bindGroups{};
    std::vector<WGPUTextureView> textureViews{};

    for (auto level = 0; level < m_numMipLevels; level++)
    {
        uint32_t levelWidth = m_textureWidth >> level;
        uint32_t levelHeight = m_textureHeight >> level;

        auto bindGroupLayout = level == 0 ? m_probabilityMapImportLevelBindGroupLayout : m_probabilityMapExportLevelBindGroupLayout;

        WGPUTextureViewDescriptor descriptor{};
        descriptor.format = WGPUTextureFormat_RGBA8Unorm;
        descriptor.dimension = WGPUTextureViewDimension_2D;
        descriptor.aspect = WGPUTextureAspect_All;
        descriptor.baseMipLevel = level;
        descriptor.mipLevelCount = 1;
        descriptor.baseArrayLayer = 0;
        descriptor.arrayLayerCount = 1;

        WGPUTextureView imageTextureView = wgpu.TextureCreateView(m_imageTexture, &descriptor);
        assert(imageTextureView);

        textureViews.push_back(imageTextureView);

        WGPUBindGroupEntry bindGroupEntries[4] = {
            WGPUBindGroupEntry{ .binding = 0, .buffer = m_probabilityMapUBOBuffer, .offset = 0, .size = sizeof(uint32_t) },
            WGPUBindGroupEntry{ .binding = 1, .buffer = level & 1 ? m_probabilityMapBufferA : m_probabilityMapBufferB, .offset = 0, .size = m_textureWidth * m_textureHeight * 4 },
            WGPUBindGroupEntry{ .binding = 2, .buffer = level & 1 ? m_probabilityMapBufferB : m_probabilityMapBufferA, .offset = 0, .size = m_textureWidth * m_textureHeight * 4 },
            WGPUBindGroupEntry{ .binding = 3, .textureView = imageTextureView },
        };

        WGPUBindGroupDescriptor bindGroupDescriptor{};
        bindGroupDescriptor.layout = bindGroupLayout;
        bindGroupDescriptor.entryCount = 4;
        bindGroupDescriptor.entries = bindGroupEntries;

        WGPUBindGroup bindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
        assert(bindGroup);

        bindGroups.push_back(bindGroup);

        if (level == 0)
        {
            WGPUComputePassDescriptor computePassDescriptor{};
            WGPUComputePassEncoder computePassEncoder = wgpu.CommandEncoderBeginComputePass(commandEncoder, &computePassDescriptor);
            wgpu.ComputePassEncoderSetPipeline(computePassEncoder, m_probabilityMapImportLevelPipeline);
            wgpu.ComputePassEncoderSetBindGroup(computePassEncoder, 0, bindGroup, 0, nullptr);
            wgpu.ComputePassEncoderDispatchWorkgroups(computePassEncoder, std::ceil(levelWidth / 64), levelHeight, 1);
            wgpu.ComputePassEncoderEnd(computePassEncoder);
        }
        else
        {
            WGPUComputePassDescriptor computePassDescriptor{};
            WGPUComputePassEncoder computePassEncoder = wgpu.CommandEncoderBeginComputePass(commandEncoder, &computePassDescriptor);
            wgpu.ComputePassEncoderSetPipeline(computePassEncoder, m_probabilityMapExportLevelPipeline);
            wgpu.ComputePassEncoderSetBindGroup(computePassEncoder, 0, bindGroup, 0, nullptr);
            wgpu.ComputePassEncoderDispatchWorkgroups(computePassEncoder, std::ceil(levelWidth / 64), levelHeight, 1);
            wgpu.ComputePassEncoderEnd(computePassEncoder);
        }
    }

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu.QueueSubmit(m_queue, 1, &commandBuffer);

    for (auto bindGroup : bindGroups)
        wgpu.BindGroupRelease(bindGroup);

    for (auto textureView : textureViews)
        wgpu.TextureViewRelease(textureView);
}

void WGPUParticlesSample::createComputeBindGroupLayout()
{
    WGPUBindGroupLayoutEntry bindGroupLayoutEntries[3] = {
        WGPUBindGroupLayoutEntry{ .binding = 0,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_Uniform } },
        WGPUBindGroupLayoutEntry{ .binding = 1,
                                  .visibility = WGPUShaderStage_Compute,
                                  .buffer = { .type = WGPUBufferBindingType_Storage } },
        WGPUBindGroupLayoutEntry{ .binding = 2,
                                  .visibility = WGPUShaderStage_Compute,
                                  .texture = { .sampleType = WGPUTextureSampleType_Float,
                                               .viewDimension = WGPUTextureViewDimension_2D,
                                               .multisampled = WGPUOptionalBool_False } },
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = 3;
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries;

    m_computeBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_computeBindGroupLayout);
}

void WGPUParticlesSample::createUniformBindGroupLayout()
{
    WGPUBindGroupLayoutEntry bindGroupEntries[1] = {
        WGPUBindGroupLayoutEntry{ .binding = 0,
                                  .visibility = WGPUShaderStage_Vertex,
                                  .buffer = { .type = WGPUBufferBindingType_Uniform } },
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = 1;
    bindGroupLayoutDescriptor.entries = bindGroupEntries;

    m_uniformBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_uniformBindGroupLayout);
}

void WGPUParticlesSample::createUniformBindGroup()
{
    WGPUBindGroupEntry bindGroupEntries[1] = {
        WGPUBindGroupEntry{ .binding = 0,
                            .buffer = m_uniformBuffer,
                            .offset = 0,
                            .size = m_uniformBufferSize },
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_uniformBindGroupLayout;
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = bindGroupEntries;

    m_uniformBindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_uniformBindGroup);
}

void WGPUParticlesSample::createComputeBindGroup()
{
    WGPUBindGroupEntry bindGroupEntries[3] = {
        WGPUBindGroupEntry{ .binding = 0,
                            .buffer = m_simulationUBOBuffer,
                            .offset = 0,
                            .size = m_simulationUBOBufferSize },
        WGPUBindGroupEntry{ .binding = 1,
                            .buffer = m_particleBuffer,
                            .offset = 0,
                            .size = static_cast<uint64_t>(m_numParticles * m_particleInstanceByteSize) },
        WGPUBindGroupEntry{ .binding = 2,
                            .textureView = m_imageTextureView },
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_computeBindGroupLayout;
    bindGroupDescriptor.entryCount = 3;
    bindGroupDescriptor.entries = bindGroupEntries;

    m_computeBindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_computeBindGroup);
}

void WGPUParticlesSample::createComputePipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_computeBindGroupLayout;

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
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_uniformBindGroupLayout;
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

    std::vector<WGPUVertexAttribute> attributes0{};
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x3;
        attribute.offset = m_particlePositionOffset;
        attribute.shaderLocation = 0;

        attributes0.push_back(attribute);
    }
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x4;
        attribute.offset = m_particleColorOffset;
        attribute.shaderLocation = 1;

        attributes0.push_back(attribute);
    }

    std::vector<WGPUVertexBufferLayout> vertexBufferLayout(2);
    vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Instance;
    vertexBufferLayout[0].attributes = attributes0.data();
    vertexBufferLayout[0].attributeCount = static_cast<uint32_t>(attributes0.size());
    vertexBufferLayout[0].arrayStride = m_particleInstanceByteSize;

    std::vector<WGPUVertexAttribute> attributes1{};
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x2;
        attribute.offset = 0;
        attribute.shaderLocation = 2;

        attributes1.push_back(attribute);
    }

    vertexBufferLayout[1].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[1].attributes = attributes1.data();
    vertexBufferLayout[1].attributeCount = static_cast<uint32_t>(attributes1.size());
    vertexBufferLayout[1].arrayStride = 2 * 4;

    std::string vertexEntryPoint = "vs_main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = vertexEntryPoint.data(), .length = vertexEntryPoint.size() };
    vertexState.module = m_wgslParticleShaderModule;
    vertexState.bufferCount = static_cast<uint32_t>(vertexBufferLayout.size());
    vertexState.buffers = vertexBufferLayout.data();

    WGPUBlendState blendState = {};
    blendState.alpha.srcFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;
    blendState.color.srcFactor = WGPUBlendFactor_Zero;
    blendState.color.dstFactor = WGPUBlendFactor_One;
    blendState.color.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceConfigure.format;
    colorTargetState.writeMask = WGPUColorWriteMask_All;
    colorTargetState.blend = &blendState;

    std::string fragEntryPoint = "fs_main";
    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = fragEntryPoint.data(), .length = fragEntryPoint.size() };
    fragState.module = m_wgslParticleShaderModule;
    fragState.targetCount = 1;
    fragState.targets = &colorTargetState;

    WGPUDepthStencilState depthStencilState{};
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.format = WGPUTextureFormat_Depth24Plus;

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_renderPipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.depthStencil = &depthStencilState;
    renderPipelineDescriptor.multisample = multisampleState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragState;

    m_renderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);

    assert(m_renderPipeline);
}

} // namespace jipu