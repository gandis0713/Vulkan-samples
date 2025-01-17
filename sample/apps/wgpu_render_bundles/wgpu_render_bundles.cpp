#include "wgpu_render_bundles.h"

#include "file.h"
#include "image.h"
#include "sphere.h"
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <spdlog/spdlog.h>

namespace jipu
{

WGPURenderBundles::WGPURenderBundles(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
    m_imgui.emplace(this);
}

WGPURenderBundles::~WGPURenderBundles()
{
    finalizeContext();
}

void WGPURenderBundles::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);
}

void WGPURenderBundles::onBeforeUpdate()
{
    WGPUSample::onBeforeUpdate();

    recordImGui({ [&]() {
        windowImGui(
            "Render Bundles", { [&]() {
                ImGui::Checkbox("Use Render Bundles", &m_useRenderBundles);
                ImGui::SliderInt("AsteroidCount", &m_asteroidCount, 0, 10000);
            } });
    } });

    if (m_currentAsteroidCount != m_asteroidCount)
    {
        m_currentAsteroidCount = m_asteroidCount;
        ensureEnoughAsteroids();
        createRenderBundle();
    }
}

void WGPURenderBundles::onUpdate()
{
    WGPUSample::onUpdate();

    {
        // glm::mat4 viewMatrix = glm::lookAt(
        //     glm::vec3(0.0f, 0.0f, 0.0f),
        //     glm::vec3(0.0f, 0.0f, -1.0f),
        //     glm::vec3(0.0f, 1.0f, 0.0f)
        // );
        glm::mat4 viewMatrix = glm::mat4(1.0f); // Identity matrix

        viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, -4.0f));

        viewMatrix = glm::rotate(viewMatrix, glm::pi<float>() * 0.1f, glm::vec3(0.0f, 0.0f, 1.0f));
        viewMatrix = glm::rotate(viewMatrix, glm::pi<float>() * 0.1f, glm::vec3(1.0f, 0.0f, 0.0f));

        // auto now = std::chrono::system_clock::now().time_since_epoch();
        // double currentTime = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count()) / 1000.0f;
        // viewMatrix = glm::rotate(viewMatrix, static_cast<float>(currentTime) * 0.00000005f, glm::vec3(0.0f, 1.0f, 0.0f));

        static float totalAngle = 0.0f;
        auto now = std::chrono::system_clock::now();
        static auto lastTime = now;
        float deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count() / 1000.0f;
        lastTime = now;
        float rotateSpeed = 0.05f;
        totalAngle += rotateSpeed * deltaTime;
        viewMatrix = glm::rotate(viewMatrix, totalAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        m_modelViewProjectionMatrix = m_projectionMatrix * viewMatrix;
    }

    wgpu.QueueWriteBuffer(m_queue, m_uniformBuffer, 0, &m_modelViewProjectionMatrix, sizeof(glm::mat4));

    ensureEnoughAsteroids();
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
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

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

    if (m_useRenderBundles)
    {
        wgpu.RenderPassEncoderExecuteBundles(renderPassEncoder, 1, &m_renderBundle);
    }
    else
    {
        // Alternatively, the same render commands can be encoded manually, which
        // can take longer since each command needs to be interpreted by the
        // JavaScript virtual machine and re-validated each time.
        renderScene(renderPassEncoder);
    }

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
    createSphereBindGroupLayout();
    createShaderModule();
    createPipelineLayout();
    createPipeline();

    {
        m_projectionMatrix = glm::perspective(
            (2.0f * glm::pi<float>()) / 5.0f,                           // FOV in radians (72 degrees = (2 * PI) / 5)
            static_cast<float>(m_width) / static_cast<float>(m_height), // Aspect ratio
            1.0f,                                                       // Near plane
            100.0f                                                      // Far plane
        );
        m_planet = createSphereRenderable(1.0);
        glm::mat4 transform{ glm::mat4(1.0) };
        m_planet.uniformBuffer = createSphereUniformBuffer(transform);
        m_planet.bindGroup = createSphereBindGroup(m_planet.uniformBuffer, m_planetImageTextureView, transform);

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

    createRenderBundle();
}

void WGPURenderBundles::finalizeContext()
{
    // TODO: check ways release and destory.

    if (m_renderBundle)
    {
        wgpu.RenderBundleRelease(m_renderBundle);
        m_renderBundle = nullptr;
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

    if (m_sphereBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_sphereBindGroupLayout);
        m_sphereBindGroupLayout = nullptr;
    }

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

    for (auto& renderable : m_renderables)
    {
        if (renderable.bindGroup)
        {
            wgpu.BindGroupRelease(renderable.bindGroup);
            renderable.bindGroup = nullptr;
        }

        if (renderable.uniformBuffer)
        {
            wgpu.BufferRelease(renderable.uniformBuffer);
            renderable.uniformBuffer = nullptr;
        }
    }
    m_renderables.clear();

    for (auto& asteroid : m_asteroids)
    {
        if (asteroid.indexBuffer)
        {
            wgpu.BufferRelease(asteroid.indexBuffer);
            asteroid.indexBuffer = nullptr;
        }

        if (asteroid.vertexBuffer)
        {
            wgpu.BufferRelease(asteroid.vertexBuffer);
            asteroid.vertexBuffer = nullptr;
        }
    }
    m_asteroids.clear();

    if (m_planet.indexBuffer)
    {
        wgpu.BufferRelease(m_planet.indexBuffer);
        m_planet.indexBuffer = nullptr;
    }

    if (m_planet.vertexBuffer)
    {
        wgpu.BufferRelease(m_planet.vertexBuffer);
        m_planet.vertexBuffer = nullptr;
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

    if (m_wgslShaderModule)
    {
        wgpu.ShaderModuleRelease(m_wgslShaderModule);
        m_wgslShaderModule = nullptr;
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
    dataLayout.rowsPerImage = height;

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
    dataLayout.rowsPerImage = height;

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
    auto uniformBufferSize = sizeof(glm::mat4);

    WGPUBufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = uniformBufferSize;
    bufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    m_uniformBuffer = wgpu.DeviceCreateBuffer(m_device, &bufferDescriptor);
    assert(m_uniformBuffer);

    // void* mappedVertexPtr = wgpu.BufferGetMappedRange(m_uniformBuffer, 0, uniformBufferSize);
    // memcpy(mappedVertexPtr, &transform, uniformBufferSize);
    // wgpu.BufferUnmap(m_uniformBuffer);

    // wgpu.QueueWriteBuffer(m_queue, m_uniformBuffer, 0, &transform, uniformBufferSize);
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

void WGPURenderBundles::createSphereBindGroupLayout()
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

    m_sphereBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_sphereBindGroupLayout);
}

void WGPURenderBundles::createBindingGroup()
{
    std::array<WGPUBindGroupEntry, 1> bindGroupEntries = {
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
    std::vector<char> shaderSource = utils::readFile(m_appDir / "render_bundles.wgsl", m_handle);

    WGPUShaderModuleWGSLDescriptor shaderModuleWGSLDescriptor{};
    shaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
    shaderModuleWGSLDescriptor.code = WGPUStringView{ .data = shaderSource.data(), .length = shaderSource.size() };

    WGPUShaderModuleDescriptor shaderModuleDescriptor{};
    shaderModuleDescriptor.nextInChain = &shaderModuleWGSLDescriptor.chain;

    m_wgslShaderModule = wgpu.DeviceCreateShaderModule(m_device, &shaderModuleDescriptor);
    assert(m_wgslShaderModule);
}

void WGPURenderBundles::createPipelineLayout()
{
    std::array<WGPUBindGroupLayout, 2> bindGroupLayouts = { m_bindGroupLayout, m_sphereBindGroupLayout };
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = bindGroupLayouts.size();
    pipelineLayoutDescriptor.bindGroupLayouts = bindGroupLayouts.data();

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
        attribute.format = WGPUVertexFormat_Float32x3;
        attribute.offset = 0;
        attribute.shaderLocation = 0;

        attributes.push_back(attribute);
    }
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x3;
        attribute.offset = sizeof(float) * 3;
        attribute.shaderLocation = 1;

        attributes.push_back(attribute);
    }
    {
        WGPUVertexAttribute attribute{};
        attribute.format = WGPUVertexFormat_Float32x2;
        attribute.offset = sizeof(float) * 6;
        attribute.shaderLocation = 2;

        attributes.push_back(attribute);
    }

    std::vector<WGPUVertexBufferLayout> vertexBufferLayout(1);
    vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayout[0].attributes = attributes.data();
    vertexBufferLayout[0].attributeCount = static_cast<uint32_t>(attributes.size());
    vertexBufferLayout[0].arrayStride = sizeof(float) * 8;

    std::string vertexEntryPoint = "vertexMain";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = vertexEntryPoint.data(), .length = vertexEntryPoint.size() };
    vertexState.module = m_wgslShaderModule;
    vertexState.bufferCount = static_cast<uint32_t>(vertexBufferLayout.size());
    vertexState.buffers = vertexBufferLayout.data();

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceConfigure.format;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    std::string fragEntryPoint = "fragmentMain";
    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = fragEntryPoint.data(), .length = fragEntryPoint.size() };
    fragState.module = m_wgslShaderModule;
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

void WGPURenderBundles::createRenderBundle()
{
    if (m_renderBundle)
    {
        wgpu.RenderBundleRelease(m_renderBundle);
        m_renderBundle = nullptr;
    }

    WGPURenderBundleEncoderDescriptor renderBundleEncoderDescriptor{};
    renderBundleEncoderDescriptor.colorFormatCount = 1;
    renderBundleEncoderDescriptor.colorFormats = &m_surfaceConfigure.format;
    renderBundleEncoderDescriptor.depthStencilFormat = WGPUTextureFormat_Depth24Plus;
    renderBundleEncoderDescriptor.depthReadOnly = false;
    renderBundleEncoderDescriptor.stencilReadOnly = false;
    renderBundleEncoderDescriptor.sampleCount = 1;

    auto renderBundleEncoder = wgpu.DeviceCreateRenderBundleEncoder(m_device, &renderBundleEncoderDescriptor);

    {
        wgpu.RenderBundleEncoderSetPipeline(renderBundleEncoder, m_renderPipeline);
        wgpu.RenderBundleEncoderSetBindGroup(renderBundleEncoder, 0, m_bindGroup, 0, nullptr);

        auto count = 0;
        for (const auto& renderable : m_renderables)
        {
            wgpu.RenderBundleEncoderSetBindGroup(renderBundleEncoder, 1, renderable.bindGroup, 0, nullptr);
            wgpu.RenderBundleEncoderSetVertexBuffer(renderBundleEncoder, 0, renderable.vertexBuffer, 0, 0);

            // uint64_t indexBufferSize = wgpu.BufferGetSize(renderable.indexBuffer);
            wgpu.RenderBundleEncoderSetIndexBuffer(renderBundleEncoder, renderable.indexBuffer, WGPUIndexFormat_Uint16, 0, renderable.indexBufferSize);
            wgpu.RenderBundleEncoderDrawIndexed(renderBundleEncoder, static_cast<uint32_t>(renderable.indexCount), 1, 0, 0, 0);

            if (++count > m_asteroidCount)
            {
                break;
            }
        }
    }

    WGPURenderBundleDescriptor renderBundleDescriptor{};
    m_renderBundle = wgpu.RenderBundleEncoderFinish(renderBundleEncoder, &renderBundleDescriptor);
}

void WGPURenderBundles::renderScene(WGPURenderPassEncoder passEncoder)
{
    wgpu.RenderPassEncoderSetPipeline(passEncoder, m_renderPipeline);
    wgpu.RenderPassEncoderSetBindGroup(passEncoder, 0, m_bindGroup, 0, nullptr);

    auto count = 0;
    for (const auto& renderable : m_renderables)
    {
        wgpu.RenderPassEncoderSetBindGroup(passEncoder, 1, renderable.bindGroup, 0, nullptr);
        wgpu.RenderPassEncoderSetVertexBuffer(passEncoder, 0, renderable.vertexBuffer, 0, 0);

        // uint64_t indexBufferSize = wgpu.BufferGetSize(renderable.indexBuffer);
        wgpu.RenderPassEncoderSetIndexBuffer(passEncoder, renderable.indexBuffer, WGPUIndexFormat_Uint16, 0, renderable.indexBufferSize);
        wgpu.RenderPassEncoderDrawIndexed(passEncoder, static_cast<uint32_t>(renderable.indexCount), 1, 0, 0, 0);

        if (++count > m_asteroidCount)
        {
            break;
        }
    }
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
        .indexBufferSize = indexBufferSize,
        .uniformBuffer = nullptr,
        .bindGroup = nullptr,
    };
}

WGPUBindGroup WGPURenderBundles::createSphereBindGroup(WGPUBuffer uniformBuffer, WGPUTextureView textureView, const glm::mat4& transform)
{
    auto uniformBufferSize = wgpu.BufferGetSize(uniformBuffer);

    std::array<WGPUBindGroupEntry, 3> bindGroupEntries = {
        WGPUBindGroupEntry{ .binding = 0, .buffer = uniformBuffer, .offset = 0, .size = uniformBufferSize },
        WGPUBindGroupEntry{ .binding = 1, .sampler = m_sampler },
        WGPUBindGroupEntry{ .binding = 2, .textureView = textureView },
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_sphereBindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    auto bindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(bindGroup);

    return bindGroup;
}

WGPUBuffer WGPURenderBundles::createSphereUniformBuffer(const glm::mat4& transform)
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

    return uniformBuffer;
}

void WGPURenderBundles::ensureEnoughAsteroids()
{
    glm::mat4 transform;
    for (size_t i = m_renderables.size(); i <= m_asteroidCount; ++i)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        float radius = dist(gen) * 1.7f + 1.25f;
        float angle = dist(gen) * 2.0f * glm::pi<float>();

        float x = std::sin(angle) * radius;
        float y = (dist(gen) - 0.5f) * 0.015f;
        float z = std::cos(angle) * radius;

        transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(x, y, z));

        float angleX = dist(gen) * glm::pi<float>();
        float angleY = dist(gen) * glm::pi<float>();

        transform = glm::rotate(transform, angleX, glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, angleY, glm::vec3(0.0f, 1.0f, 0.0f));

        auto renderable = m_asteroids[i % m_asteroids.size()];
        renderable.uniformBuffer = createSphereUniformBuffer(transform);
        renderable.bindGroup = createSphereBindGroup(renderable.uniformBuffer, m_moonImageTextureView, transform);
        m_renderables.push_back(renderable);
    }
}

} // namespace jipu