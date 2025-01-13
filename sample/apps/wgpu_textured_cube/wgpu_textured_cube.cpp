#include "wgpu_textured_cube.h"

#include "file.h"
#include "image.h"
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUTexturedCube::WGPUTexturedCube(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
    m_imgui.emplace(this);
}

WGPUTexturedCube::~WGPUTexturedCube()
{
    finalizeContext();
}

void WGPUTexturedCube::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);
}

void WGPUTexturedCube::onUpdate()
{
    WGPUSample::onUpdate();

    auto transformationMatrix = [&]() -> glm::mat4 {
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 100.0f);

        glm::mat4 viewMatrix = glm::mat4(1.0f); // Identity matrix

        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -4.0f));

        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> seconds = now.time_since_epoch();
        auto currentTime = seconds.count();

        glm::vec3 rotationAxis(std::sin(currentTime), std::cos(currentTime), 0.0f);
        viewMatrix = glm::rotate(viewMatrix, 1.0f, rotationAxis);

        glm::mat4 modelViewProjectionMatrix = projectionMatrix * viewMatrix;

        return modelViewProjectionMatrix;
    }();

    wgpu.QueueWriteBuffer(m_queue, m_uniformBuffer, 0, &transformationMatrix, sizeof(glm::mat4));
}

void WGPUTexturedCube::onDraw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);
    WGPUTextureView depthTextureView = wgpu.TextureCreateView(m_depthTexture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.5f, .g = 0.5f, .b = 0.5f, .a = 1.0f };

    WGPURenderPassDepthStencilAttachment depthStencilAttachment{};
    depthStencilAttachment.view = depthTextureView;
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthClearValue = 1.0f;

    WGPURenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;
    renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;

    WGPURenderPassEncoder renderPassEncoder = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpu.RenderPassEncoderSetPipeline(renderPassEncoder, m_renderPipeline);
    wgpu.RenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f);
    wgpu.RenderPassEncoderSetScissorRect(renderPassEncoder, 0, 0, m_width, m_height);
    wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_cubeVertexBuffer, 0, m_cube.size() * sizeof(float));
    wgpu.RenderPassEncoderSetBindGroup(renderPassEncoder, 0, m_bindGroup, 0, nullptr);
    // wgpu.RenderPassEncoderSetIndexBuffer(renderPassEncoder, m_cubeIndexBuffer, WGPUIndexFormat_Uint16, 0, m_indices.size() * sizeof(IndexType));
    // wgpu.RenderPassEncoderDrawIndexed(renderPassEncoder, 3, 1, 0, 0, 0);
    wgpu.RenderPassEncoderDraw(renderPassEncoder, 36, 1, 0, 0);
    wgpu.RenderPassEncoderEnd(renderPassEncoder);
    wgpu.RenderPassEncoderRelease(renderPassEncoder);

    drawImGui(commandEncoder, surfaceTextureView);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu.QueueSubmit(m_queue, 1, &commandBuffer);
    wgpu.SurfacePresent(m_surface);

    wgpu.CommandBufferRelease(commandBuffer);
    wgpu.CommandEncoderRelease(commandEncoder);
    wgpu.TextureViewRelease(depthTextureView);
    wgpu.TextureViewRelease(surfaceTextureView);
    wgpu.TextureRelease(surfaceTexture.texture);
}

void WGPUTexturedCube::initializeContext()
{
    WGPUSample::initializeContext();

    createCubeBuffer();
    createDepthTexture();
    createImageTexture();
    createImageTextureView();
    createSampler();
    createUniformBuffer();
    createBindingGroupLayout();
    createBindingGroup();
    createShaderModule();
    createPipelineLayout();
    createPipeline();
}

void WGPUTexturedCube::finalizeContext()
{
    // TODO: check ways release and destory.
    if (m_cubeVertexBuffer)
    {
        wgpu.BufferRelease(m_cubeVertexBuffer);
        m_cubeVertexBuffer = nullptr;
    }

    // if (m_cubeIndexBuffer)
    // {
    //     wgpu.BufferRelease(m_cubeIndexBuffer);
    //     m_cubeIndexBuffer = nullptr;
    // }

    if (m_uniformBuffer)
    {
        wgpu.BufferRelease(m_uniformBuffer);
        m_uniformBuffer = nullptr;
    }

    if (m_sampler)
    {
        wgpu.SamplerRelease(m_sampler);
        m_sampler = nullptr;
    }

    if (m_imageTextureView)
    {
        wgpu.TextureViewRelease(m_imageTextureView);
        m_imageTextureView = nullptr;
    }

    if (m_imageTexture)
    {
        wgpu.TextureRelease(m_imageTexture);
        m_imageTexture = nullptr;
    }

    if (m_depthTexture)
    {
        wgpu.TextureRelease(m_depthTexture);
        m_depthTexture = nullptr;
    }

    if (m_bindGroup)
    {
        wgpu.BindGroupRelease(m_bindGroup);
        m_bindGroup = nullptr;
    }

    if (m_bindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_bindGroupLayout);
        m_bindGroupLayout = nullptr;
    }

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

void WGPUTexturedCube::createCubeBuffer()
{
    {
        size_t vertexBufferSize = m_cube.size() * sizeof(float);
        WGPUBufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = vertexBufferSize;
        bufferDescriptor.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
        // bufferDescriptor.mappedAtCreation = true;

        m_cubeVertexBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
        assert(m_cubeVertexBuffer);
        // void* mappedVertexPtr = wgpu.BufferGetMappedRange(m_cubeVertexBuffer, 0, vertexBufferSize);
        // memcpy(mappedVertexPtr, m_cube.data(), vertexBufferSize);
        // wgpu.BufferUnmap(m_cubeVertexBuffer);

        wgpu.QueueWriteBuffer(m_queue, m_cubeVertexBuffer, 0, m_cube.data(), vertexBufferSize);
    }

    // {
    //     size_t indexBufferSize = m_indices.size() * sizeof(IndexType);
    //     WGPUBufferDescriptor bufferDescriptor{};
    //     bufferDescriptor.size = indexBufferSize;
    //     bufferDescriptor.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
    //     // bufferDescriptor.mappedAtCreation = true;

    //     m_cubeIndexBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    //     assert(m_cubeIndexBuffer);

    //     // void* mappedIndexPtr = wgpu.BufferGetMappedRange(m_cubeIndexBuffer, 0, indexBufferSize);
    //     // memcpy(mappedIndexPtr, m_indices.data(), indexBufferSize);
    //     // wgpu.BufferUnmap(m_cubeIndexBuffer);

    //     wgpu.QueueWriteBuffer(m_queue, m_cubeIndexBuffer, 0, m_indices.data(), indexBufferSize);
    // }
}

void WGPUTexturedCube::createDepthTexture()
{
    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size.width = m_width;
    descriptor.size.height = m_height;
    descriptor.size.depthOrArrayLayers = 1;
    descriptor.sampleCount = 1;
    descriptor.format = WGPUTextureFormat_Depth24Plus;
    descriptor.mipLevelCount = 1;
    descriptor.usage = WGPUTextureUsage_RenderAttachment;

    m_depthTexture = wgpu.DeviceCreateTexture(m_device, &descriptor);
    assert(m_depthTexture);
}

void WGPUTexturedCube::createImageTexture()
{
    std::vector<char> buffer = utils::readFile(m_appDir / "Di-3d.png", m_handle);
    auto image = std::make_unique<Image>(buffer.data(), buffer.size());

    unsigned char* pixels = static_cast<unsigned char*>(image->getPixels());
    uint32_t width = image->getWidth();
    uint32_t height = image->getHeight();
    uint32_t channel = image->getChannel();
    uint64_t imageSize = sizeof(unsigned char) * width * height * channel;
    uint32_t mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    if (mipLevelCount > 10)
        mipLevelCount = 10;

    WGPUTextureDescriptor descriptor{};
    descriptor.dimension = WGPUTextureDimension_2D;
    descriptor.size.width = width;
    descriptor.size.height = height;
    descriptor.size.depthOrArrayLayers = 1;
    descriptor.sampleCount = 1;
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.mipLevelCount = mipLevelCount;
    descriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

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

void WGPUTexturedCube::createImageTextureView()
{
    WGPUTextureViewDescriptor descriptor{};
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.dimension = WGPUTextureViewDimension_2D;
    descriptor.aspect = WGPUTextureAspect_All;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = 1;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;

    m_imageTextureView = wgpu.TextureCreateView(m_imageTexture, &descriptor);
    assert(m_imageTextureView);
}

void WGPUTexturedCube::createSampler()
{
    WGPUSamplerDescriptor samplerDescriptor{};
    samplerDescriptor.minFilter = WGPUFilterMode_Linear;
    samplerDescriptor.magFilter = WGPUFilterMode_Linear;
    samplerDescriptor.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDescriptor.addressModeU = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.addressModeV = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDescriptor.lodMinClamp = 0.0f;
    samplerDescriptor.lodMaxClamp = 1.0f;
    samplerDescriptor.compare = WGPUCompareFunction_Undefined;
    samplerDescriptor.maxAnisotropy = 1;

    m_sampler = wgpu.DeviceCreateSampler(m_device, &samplerDescriptor);
    assert(m_sampler);
}

void WGPUTexturedCube::createUniformBuffer()
{
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = sizeof(glm::mat4);
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_uniformBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    assert(m_uniformBuffer);
}
void WGPUTexturedCube::createBindingGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 3> bindGroupLayoutEntries = {
        WGPUBindGroupLayoutEntry{ .binding = 0,
                                  .visibility = WGPUShaderStage_Vertex,
                                  .buffer = { .type = WGPUBufferBindingType_Uniform } },
        WGPUBindGroupLayoutEntry{ .binding = 1,
                                  .visibility = WGPUShaderStage_Fragment,
                                  .sampler = { .type = WGPUSamplerBindingType_Filtering } },
        WGPUBindGroupLayoutEntry{ .binding = 2,
                                  .visibility = WGPUShaderStage_Fragment,
                                  .texture = { .sampleType = WGPUTextureSampleType_Float,
                                               .viewDimension = WGPUTextureViewDimension_2D,
                                               .multisampled = WGPUOptionalBool_False } },
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_bindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_bindGroupLayout);
}

void WGPUTexturedCube::createBindingGroup()
{
    std::array<WGPUBindGroupEntry, 3> bindGroupEntries = {
        WGPUBindGroupEntry{ .binding = 0, .buffer = m_uniformBuffer, .offset = 0, .size = sizeof(glm::mat4) },
        WGPUBindGroupEntry{ .binding = 1, .sampler = m_sampler },
        WGPUBindGroupEntry{ .binding = 2, .textureView = m_imageTextureView },
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_bindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    m_bindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_bindGroup);
}

void WGPUTexturedCube::createShaderModule()
{
    std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "textured_cube.vert.wgsl", m_handle);
    std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "textured_cube.frag.wgsl", m_handle);

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

void WGPUTexturedCube::createPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_bindGroupLayout;

    m_pipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_pipelineLayout);
}

void WGPUTexturedCube::createPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_Back;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    std::vector<WGPUVertexAttribute> attributes{};
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x4;
        attribute.offset = 0;
        attribute.shaderLocation = 0;

        attributes.push_back(attribute);
    }
    // {
    //     WGPUVertexAttribute attribute{};
    //     attribute.format = WGPUVertexFormat_Float32x4;
    //     attribute.offset = sizeof(float) * 4;
    //     attribute.shaderLocation = 0;

    //     attributes.push_back(attribute);
    // }
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x2;
        attribute.offset = sizeof(float) * 8;
        attribute.shaderLocation = 1;

        attributes.push_back(attribute);
    }

    std::vector<WGPUVertexBufferLayout> vertexBufferLayout(1);
    vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[0].attributes = attributes.data();
    vertexBufferLayout[0].attributeCount = static_cast<uint32_t>(attributes.size());
    vertexBufferLayout[0].arrayStride = sizeof(float) * 10;

    std::string entryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    vertexState.module = m_vertWGSLShaderModule;
    vertexState.bufferCount = static_cast<uint32_t>(vertexBufferLayout.size());
    vertexState.buffers = vertexBufferLayout.data();

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceConfigure.format;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    fragState.module = m_fragWGSLShaderModule;
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
    renderPipelineDescriptor.layout = m_pipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.multisample = multisampleState;
    renderPipelineDescriptor.depthStencil = &depthStencilState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragState;

    m_renderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);

    assert(m_renderPipeline);
}

} // namespace jipu