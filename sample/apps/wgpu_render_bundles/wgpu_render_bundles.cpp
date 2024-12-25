#include "wgpu_render_bundles.h"

#include "file.h"
#include "image.h"
#include "sphere.h"
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

WGPURenderBundles::WGPURenderBundles(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
}

WGPURenderBundles::~WGPURenderBundles()
{
    finalizeContext();
}

void WGPURenderBundles::init()
{
    WGPUSample::init();

    // changeAPI(APIType::kJipu);
    changeAPI(APIType::kDawn);
}

void WGPURenderBundles::onUpdate()
{
    WGPUSample::onUpdate();

    {
        // const viewMatrix = mat4.identity();
        auto viewMatrix = glm::identity<glm::mat4>();

        // mat4.translate(viewMatrix, vec3.fromValues(0, 0, -4), viewMatrix);
        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -4.0f));

        // const now = Date.now() / 1000;
        auto now = std::chrono::high_resolution_clock::now();

        // Tilt the view matrix so the planet looks like it's off-axis.
        // mat4.rotateZ(viewMatrix, Math.PI * 0.1, viewMatrix);
        viewMatrix = glm::rotate(viewMatrix, glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // mat4.rotateX(viewMatrix, Math.PI * 0.1, viewMatrix);
        viewMatrix = glm::rotate(viewMatrix, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        // Rotate the view matrix slowly so the planet appears to spin.
        // mat4.rotateY(viewMatrix, now * 0.05, viewMatrix);
        viewMatrix = glm::rotate(viewMatrix, static_cast<float>(now.time_since_epoch().count() * 0.05), glm::vec3(0.0f, 1.0f, 0.0f));

        // mat4.multiply(projectionMatrix, viewMatrix, modelViewProjectionMatrix);
        m_modelViewProjectionMatrix = m_projectionMatrix * viewMatrix;
    }

    wgpu.QueueWriteBuffer(m_queue, m_uniformBuffer, 0, &m_modelViewProjectionMatrix, sizeof(glm::mat4));
}

void WGPURenderBundles::onDraw()
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
    // wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_cubeVertexBuffer, 0, m_cube.size() * sizeof(float));
    wgpu.RenderPassEncoderSetBindGroup(renderPassEncoder, 0, m_bindGroup, 0, nullptr);
    wgpu.RenderPassEncoderDraw(renderPassEncoder, 36, 1, 0, 0);
    wgpu.RenderPassEncoderEnd(renderPassEncoder);
    wgpu.RenderPassEncoderRelease(renderPassEncoder);

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

void WGPURenderBundles::initializeContext()
{
    WGPUSample::initializeContext();

    createDepthTexture();
    createMoonImageTexture();
    createMoonImageTextureView();
    createPlanetImageTexture();
    createPlanetImageTextureView();
    createSampler();
    createUniformBuffer();
    createBindingGroupLayout();
    createBindingGroup();
    createShaderModule();
    createPipelineLayout();
    createPipeline();

    {
        m_projectionMatrix = glm::perspective(
            glm::radians(72.0f),                    // FOV in radians (72 degrees = (2 * PI) / 5)
            m_width / static_cast<float>(m_height), // Aspect ratio
            1.0f,                                   // Near plane
            100.0f                                  // Far plane
        );
        m_planet = createSphereRenderable(1.0);
        m_planet.bindGroup = createSphereBindGroup(m_planetImageTextureView, m_transform);

        m_asteroids = {
            createSphereRenderable(0.01, 8, 6, 0.15),
            createSphereRenderable(0.013, 8, 6, 0.15),
            createSphereRenderable(0.017, 8, 6, 0.15),
            createSphereRenderable(0.02, 8, 6, 0.15),
            createSphereRenderable(0.03, 16, 8, 0.15),
        };

        m_renderables = { m_planet };

        ensureEnoughAsteroids();
    }
}

void WGPURenderBundles::finalizeContext()
{
    // TODO: check ways release and destory.

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

    if (m_planetImageTextureView)
    {
        wgpu.TextureViewRelease(m_planetImageTextureView);
        m_planetImageTextureView = nullptr;
    }

    if (m_planetImageTexture)
    {
        wgpu.TextureRelease(m_planetImageTexture);
        m_planetImageTexture = nullptr;
    }

    if (m_moonImageTextureView)
    {
        wgpu.TextureViewRelease(m_moonImageTextureView);
        m_moonImageTextureView = nullptr;
    }

    if (m_moonImageTexture)
    {
        wgpu.TextureRelease(m_moonImageTexture);
        m_moonImageTexture = nullptr;
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

void WGPURenderBundles::createDepthTexture()
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

void WGPURenderBundles::createMoonImageTexture()
{
    std::vector<char> buffer = utils::readFile(m_appDir / "moon.jpg", m_handle);
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

    m_moonImageTexture = wgpu.DeviceCreateTexture(m_device, &descriptor);
    assert(m_moonImageTexture);

    WGPUImageCopyTexture imageCopyTexture{};
    imageCopyTexture.texture = m_moonImageTexture;
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

void WGPURenderBundles::createMoonImageTextureView()
{
    WGPUTextureViewDescriptor descriptor{};
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.dimension = WGPUTextureViewDimension_2D;
    descriptor.aspect = WGPUTextureAspect_All;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = 1;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;

    m_moonImageTextureView = wgpu.TextureCreateView(m_moonImageTexture, &descriptor);
    assert(m_moonImageTextureView);
}

void WGPURenderBundles::createPlanetImageTexture()
{
    std::vector<char> buffer = utils::readFile(m_appDir / "saturn.jpg", m_handle);
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

    m_planetImageTexture = wgpu.DeviceCreateTexture(m_device, &descriptor);
    assert(m_planetImageTexture);

    WGPUImageCopyTexture imageCopyTexture{};
    imageCopyTexture.texture = m_planetImageTexture;
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

void WGPURenderBundles::createPlanetImageTextureView()
{
    WGPUTextureViewDescriptor descriptor{};
    descriptor.format = WGPUTextureFormat_RGBA8Unorm;
    descriptor.dimension = WGPUTextureViewDimension_2D;
    descriptor.aspect = WGPUTextureAspect_All;
    descriptor.baseMipLevel = 0;
    descriptor.mipLevelCount = 1;
    descriptor.baseArrayLayer = 0;
    descriptor.arrayLayerCount = 1;

    m_planetImageTextureView = wgpu.TextureCreateView(m_planetImageTexture, &descriptor);
    assert(m_planetImageTextureView);
}

void WGPURenderBundles::createSampler()
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

void WGPURenderBundles::createUniformBuffer()
{
    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = sizeof(glm::mat4);
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_uniformBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    assert(m_uniformBuffer);
}
void WGPURenderBundles::createBindingGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 1> bindGroupLayoutEntries = {
        WGPUBindGroupLayoutEntry{ .binding = 0,
                                  .visibility = WGPUShaderStage_Vertex,
                                  .buffer = { .type = WGPUBufferBindingType_Uniform } }
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_bindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_bindGroupLayout);
}

void WGPURenderBundles::createBindingGroup()
{
    std::array<WGPUBindGroupEntry, 3> bindGroupEntries = {
        WGPUBindGroupEntry{ .binding = 0, .buffer = m_uniformBuffer, .offset = 0, .size = sizeof(glm::mat4) },
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_bindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    m_bindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_bindGroup);
}

void WGPURenderBundles::createShaderModule()
{
    std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "render_bundles.vert.wgsl", m_handle);
    std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "render_bundles.frag.wgsl", m_handle);

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

void WGPURenderBundles::createPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_bindGroupLayout;

    m_pipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_pipelineLayout);
}

void WGPURenderBundles::createPipeline()
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
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x3;
        attribute.offset = sizeof(float) * 4;
        attribute.shaderLocation = 1;

        attributes.push_back(attribute);
    }
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x2;
        attribute.offset = sizeof(float) * 7;
        attribute.shaderLocation = 2;

        attributes.push_back(attribute);
    }

    std::vector<WGPUVertexBufferLayout> vertexBufferLayout(1);
    vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[0].attributes = attributes.data();
    vertexBufferLayout[0].attributeCount = static_cast<uint32_t>(attributes.size());
    vertexBufferLayout[0].arrayStride = sizeof(float) * 9;

    std::string entryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    vertexState.module = m_vertWGSLShaderModule;
    vertexState.bufferCount = static_cast<uint32_t>(vertexBufferLayout.size());
    vertexState.buffers = vertexBufferLayout.data();

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceCapabilities.formats[0];
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

WGPURenderBundles::Renderable WGPURenderBundles::createSphereRenderable(float radius, int widthSegments, int heightSegments, float randomness)
{
    SphereMesh sphereMesh = createSphereMesh(radius,
                                             widthSegments,
                                             heightSegments,
                                             randomness);

    size_t vertexBufferSize = sphereMesh.vertices.size() * sizeof(float);
    size_t indexBufferSize = sphereMesh.indices.size() * sizeof(uint16_t);

    // Create a vertex buffer from the sphere data.
    WGPUBufferDescriptor vertexBufferDescriptor{};
    vertexBufferDescriptor.size = vertexBufferSize;
    vertexBufferDescriptor.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    vertexBufferDescriptor.mappedAtCreation = false;

    auto vertexBuffer = wgpu.DeviceCreateBuffer(m_device, &vertexBufferDescriptor);
    assert(vertexBuffer);

    // void* mappedVertexPtr = wgpu.BufferGetMappedRange(vertexBuffer, 0, vertexBufferSize);
    // memcpy(mappedVertexPtr, sphereMesh.vertices.data(), vertexBufferSize);
    // wgpu.BufferUnmap(vertexBuffer);

    wgpu.QueueWriteBuffer(m_queue, vertexBuffer, 0, sphereMesh.vertices.data(), vertexBufferSize);

    WGPUBufferDescriptor indexBufferDescriptor{};
    indexBufferDescriptor.size = indexBufferSize;
    indexBufferDescriptor.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
    indexBufferDescriptor.mappedAtCreation = false;

    auto indexBuffer = wgpu.DeviceCreateBuffer(m_device, &indexBufferDescriptor);
    assert(indexBuffer);

    // void* mappedVertexPtr = wgpu.BufferGetMappedRange(indexBuffer, 0, indexBufferSize);
    // memcpy(mappedVertexPtr, sphereMesh.indices.data(), indexBufferSize);
    // wgpu.BufferUnmap(indexBuffer);

    wgpu.QueueWriteBuffer(m_queue, indexBuffer, 0, sphereMesh.indices.data(), indexBufferSize);

    return Renderable{
        .vertexBuffer = vertexBuffer,
        .indexBuffer = indexBuffer,
        .indexCount = sphereMesh.indices.size(),
        .bindGroup = nullptr
    };
}

WGPUBindGroup WGPURenderBundles::createSphereBindGroup(WGPUTextureView textureView, const glm::mat4& transform)
{
    auto uniformBufferSize = sizeof(glm::mat4); // 4x4 matrix

    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = uniformBufferSize;
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    auto uniformBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    assert(uniformBuffer);

    // void* mappedVertexPtr = wgpu.BufferGetMappedRange(uniformBuffer, 0, uniformBufferSize);
    // memcpy(mappedVertexPtr, &transform, uniformBufferSize);
    // wgpu.BufferUnmap(uniformBuffer);

    wgpu.QueueWriteBuffer(m_queue, uniformBuffer, 0, &transform, uniformBufferSize);

    std::array<WGPUBindGroupEntry, 3> bindGroupEntries = {
        WGPUBindGroupEntry{ .binding = 0, .buffer = uniformBuffer, .offset = 0, .size = uniformBufferSize },
        WGPUBindGroupEntry{ .binding = 1, .sampler = m_sampler },
        WGPUBindGroupEntry{ .binding = 2, .textureView = textureView },
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_bindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    auto bindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(bindGroup);

    return bindGroup;
}

void WGPURenderBundles::ensureEnoughAsteroids()
{
    auto asteroidCount = 1000; // TODO: by settings

    auto transform = glm::identity<glm::mat4>();
    for (size_t i = m_renderables.size(); i <= asteroidCount; ++i)
    {
        // Place copies of the asteroid in a ring.
        float radius = static_cast<float>(rand()) / RAND_MAX * 1.7f + 1.25f;
        float angle = static_cast<float>(rand()) / RAND_MAX * glm::two_pi<float>();
        float x = std::sin(angle) * radius;
        float y = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.015f;
        float z = std::cos(angle) * radius;

        transform = glm::translate(transform, glm::vec3(x, y, z));
        transform = glm::rotate(transform, static_cast<float>(rand()) / RAND_MAX * glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, static_cast<float>(rand()) / RAND_MAX * glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));

        auto renderable = m_asteroids[i % m_asteroids.size()];
        renderable.bindGroup = createSphereBindGroup(m_moonImageTextureView, transform);
        m_renderables.push_back(renderable);
    }
}

} // namespace jipu