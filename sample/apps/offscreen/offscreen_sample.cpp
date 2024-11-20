

#include "offscreen_sample.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

OffscreenSample::OffscreenSample(const SampleDescriptor& descriptor)
    : Sample(descriptor)
{
}

OffscreenSample::~OffscreenSample()
{
    m_offscreen.renderPipeline.reset();
    m_offscreen.renderPipelineLayout.reset();
    m_offscreen.bindGroup.reset();
    m_offscreen.bindGroupLayout.reset();
    m_offscreen.vertexBuffer.reset();
    m_offscreen.indexBuffer.reset();
    m_offscreen.uniformBuffer.reset();
    m_offscreen.renderTextureView.reset();
    m_offscreen.renderTexture.reset();

    m_onscreen.renderPipeline.reset();
    m_onscreen.renderPipelineLayout.reset();
    m_onscreen.bindGroup.reset();
    m_onscreen.bindGroupLayout.reset();
    m_onscreen.vertexBuffer.reset();
    m_onscreen.indexBuffer.reset();
    m_onscreen.sampler.reset();
}

void OffscreenSample::init()
{
    Sample::init();

    createHPCWatcher();

    createOffscreenTexture();
    createOffscreenTextureView();
    createOffscreenVertexBuffer();
    createOffscreenIndexBuffer();
    createOffscreenUniformBuffer();
    createOffscreenBindGroupLayout();
    createOffscreenBindGroup();
    createOffscreenRenderPipeline();

    createOnscreenVertexBuffer();
    createOnscreenIndexBuffer();
    createOnscreenSampler();
    createOnscreenBindGroupLayout();
    createOnscreenBindGroup();
    createOnscreenRenderPipeline();

    createCamera();
}

void OffscreenSample::createCamera()
{
    m_camera = std::make_unique<PerspectiveCamera>(45.0f,
                                                   m_width / static_cast<float>(m_height),
                                                   0.1f,
                                                   1000.0f);

    // auto halfWidth = m_width / 2.0f;
    // auto halfHeight = m_height / 2.0f;
    // m_camera = std::make_unique<OrthographicCamera>(-halfWidth, halfWidth,
    //                                                 -halfHeight, halfHeight,
    //                                                 -1000, 1000);

    m_camera->lookAt(glm::vec3(0.0f, 0.0f, 1000.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0f, 0.0));
}

void OffscreenSample::updateOffscreenUniformBuffer()
{
    m_ubo.mvp.model = glm::mat4(1.0f);
    m_ubo.mvp.view = m_camera->getViewMat();
    m_ubo.mvp.proj = m_camera->getProjectionMat();

    void* pointer = m_offscreen.uniformBuffer->map(); // do not unmap.
    memcpy(pointer, &m_ubo, m_offscreen.uniformBuffer->getSize());
}

void OffscreenSample::onUpdate()
{
    Sample::onUpdate();

    updateOffscreenUniformBuffer();

    updateImGui();
}

void OffscreenSample::onDraw()
{
    auto renderView = m_swapchain->acquireNextTextureView();

    // offscreen pass
    {
        ColorAttachment attachment{
            .renderView = m_offscreen.renderTextureView.get()
        };
        attachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        attachment.loadOp = LoadOp::kClear;
        attachment.storeOp = StoreOp::kStore;

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { attachment }
        };

        CommandEncoderDescriptor commandDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandDescriptor);

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_offscreen.renderPipeline.get());
        renderPassEncoder->setBindGroup(0, m_offscreen.bindGroup.get());
        renderPassEncoder->setVertexBuffer(0, m_offscreen.vertexBuffer.get());
        renderPassEncoder->setIndexBuffer(m_offscreen.indexBuffer.get(), IndexFormat::kUint16);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_offscreenIndices.size()), 1, 0, 0, 0);
        renderPassEncoder->end();

        auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
        m_queue->submit({ commandBuffer.get() });
    }

    // onscreen pass
    {
        ColorAttachment attachment{
            .renderView = renderView
        };
        attachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        attachment.loadOp = LoadOp::kClear;
        attachment.storeOp = StoreOp::kStore;

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { attachment }
        };

        CommandEncoderDescriptor commandDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandDescriptor);

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_onscreen.renderPipeline.get());
        renderPassEncoder->setBindGroup(0, m_onscreen.bindGroup.get());
        renderPassEncoder->setVertexBuffer(0, m_onscreen.vertexBuffer.get());
        renderPassEncoder->setIndexBuffer(m_onscreen.indexBuffer.get(), IndexFormat::kUint16);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_onscreenIndices.size()), 1, 0, 0, 0);
        renderPassEncoder->end();

        drawImGui(commandEncoder.get(), renderView);

        auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
        m_queue->submit({ commandBuffer.get() });
        m_swapchain->present();
    }
}

void OffscreenSample::updateImGui()
{
    recordImGui({ [&]() {
        profilingWindow();
    } });
}

void OffscreenSample::createOffscreenTexture()
{
#if defined(__ANDROID__) || defined(ANDROID)
    TextureFormat textureFormat = TextureFormat::kRGBA8Unorm;
#else
    TextureFormat textureFormat = TextureFormat::kBGRA8Unorm;
#endif

    TextureDescriptor textureDescriptor;
    textureDescriptor.width = m_width;
    textureDescriptor.height = m_height;
    textureDescriptor.depth = 1;
    textureDescriptor.format = textureFormat;
    textureDescriptor.usage = TextureUsageFlagBits::kColorAttachment | TextureUsageFlagBits::kTextureBinding;
    textureDescriptor.type = TextureType::k2D;
    textureDescriptor.sampleCount = 1; // TODO: set from descriptor
    textureDescriptor.mipLevels = 1;   // TODO: set from descriptor

    m_offscreen.renderTexture = m_device->createTexture(textureDescriptor);
}

void OffscreenSample::createOffscreenTextureView()
{
    TextureViewDescriptor textureViewDescriptor;
    textureViewDescriptor.aspect = TextureAspectFlagBits::kColor;
    textureViewDescriptor.dimension = TextureViewDimension::k2D;

    m_offscreen.renderTextureView = m_offscreen.renderTexture->createTextureView(textureViewDescriptor);
}

void OffscreenSample::createOffscreenVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_offscreenVertices.size() * sizeof(OffscreenVertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_offscreen.vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_offscreen.vertexBuffer->map();
    memcpy(pointer, m_offscreenVertices.data(), descriptor.size);
    m_offscreen.vertexBuffer->unmap();
}

void OffscreenSample::createOffscreenIndexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_offscreenIndices.size() * sizeof(uint16_t);
    descriptor.usage = BufferUsageFlagBits::kIndex;

    m_offscreen.indexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_offscreen.indexBuffer->map();
    memcpy(pointer, m_offscreenIndices.data(), descriptor.size);
    m_offscreen.indexBuffer->unmap();
}

void OffscreenSample::createOffscreenUniformBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = sizeof(UBO);
    descriptor.usage = BufferUsageFlagBits::kUniform;

    m_offscreen.uniformBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_offscreen.uniformBuffer->map();
    // memcpy(pointer, &m_ubo, descriptor.size);
    // m_offscreen.uniformBuffer->unmap();
}

void OffscreenSample::createOffscreenBindGroupLayout()
{
    BufferBindingLayout bufferLayout{};
    bufferLayout.index = 0;
    bufferLayout.stages = BindingStageFlagBits::kVertexStage;
    bufferLayout.type = BufferBindingType::kUniform;

    BindGroupLayoutDescriptor descriptor{};
    descriptor.buffers = { bufferLayout };

    m_offscreen.bindGroupLayout = m_device->createBindGroupLayout(descriptor);
}

void OffscreenSample::createOffscreenBindGroup()
{
    BufferBinding bufferBinding{
        .index = 0,
        .offset = 0,
        .size = m_offscreen.uniformBuffer->getSize(),
        .buffer = m_offscreen.uniformBuffer.get(),
    };

    BindGroupDescriptor descriptor{
        .layout = m_offscreen.bindGroupLayout.get(),
        .buffers = { bufferBinding }
    };

    m_offscreen.bindGroup = m_device->createBindGroup(descriptor);
}

void OffscreenSample::createOffscreenRenderPipeline()
{
    // render pipeline layout
    {
        PipelineLayoutDescriptor descriptor{};
        descriptor.layouts = { m_offscreen.bindGroupLayout.get() };

        m_offscreen.renderPipelineLayout = m_device->createPipelineLayout(descriptor);
    }

    // input assembly stage
    InputAssemblyStage inputAssemblyStage{};
    {
        inputAssemblyStage.topology = PrimitiveTopology::kTriangleList;
    }

    // vertex shader module
    std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "offscreen.vert.spv", m_handle);
        descriptor.code = vertexShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(vertexShaderSource.size());

        vertexShaderModule = m_device->createShaderModule(descriptor);
    }

    // vertex stage

    VertexAttribute positionAttribute{};
    positionAttribute.format = VertexFormat::kFloat32x3;
    positionAttribute.offset = offsetof(OffscreenVertex, pos);
    positionAttribute.location = 0;

    VertexAttribute colorAttribute{};
    colorAttribute.format = VertexFormat::kFloat32x3;
    colorAttribute.offset = offsetof(OffscreenVertex, color);
    colorAttribute.location = 1;

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(OffscreenVertex);
    vertexInputLayout.attributes = { positionAttribute, colorAttribute };

    VertexStage vertexStage{
        { vertexShaderModule.get(), "main" },
        { vertexInputLayout }
    };

    // rasterization
    RasterizationStage rasterizationStage{};
    {
        rasterizationStage.cullMode = CullMode::kNone;
        rasterizationStage.frontFace = FrontFace::kCounterClockwise;
        rasterizationStage.sampleCount = m_sampleCount;
    }

    // fragment shader module
    std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "offscreen.frag.spv", m_handle);
        descriptor.code = fragmentShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(fragmentShaderSource.size());

        fragmentShaderModule = m_device->createShaderModule(descriptor);
    }

    // fragment

    FragmentStage::Target target{};
    target.format = m_offscreen.renderTexture->getFormat();

    FragmentStage fragmentStage{
        { fragmentShaderModule.get(), "main" }, { target }
    };

    // depth/stencil

    // render pipeline
    RenderPipelineDescriptor descriptor{
        m_offscreen.renderPipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    m_offscreen.renderPipeline = m_device->createRenderPipeline(descriptor);
}

void OffscreenSample::createOnscreenVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_onscreenVertices.size() * sizeof(OnscreenVertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_onscreen.vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_onscreen.vertexBuffer->map();
    memcpy(pointer, m_onscreenVertices.data(), descriptor.size);
    m_onscreen.vertexBuffer->unmap();
}

void OffscreenSample::createOnscreenIndexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_onscreenIndices.size() * sizeof(uint16_t);
    descriptor.usage = BufferUsageFlagBits::kIndex;

    m_onscreen.indexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_onscreen.indexBuffer->map();
    memcpy(pointer, m_onscreenIndices.data(), descriptor.size);
    m_onscreen.indexBuffer->unmap();
}

void OffscreenSample::createOnscreenSampler()
{
    SamplerDescriptor samplerDescriptor{};
    samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
    samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
    samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
    samplerDescriptor.lodMin = 0.0f;
    samplerDescriptor.lodMax = static_cast<float>(m_offscreen.renderTexture->getMipLevels());
    samplerDescriptor.minFilter = FilterMode::kLinear;
    samplerDescriptor.magFilter = FilterMode::kLinear;
    samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;

    m_onscreen.sampler = m_device->createSampler(samplerDescriptor);
}

void OffscreenSample::createOnscreenBindGroupLayout()
{
    SamplerBindingLayout samplerLayout{};
    samplerLayout.index = 0;
    samplerLayout.stages = BindingStageFlagBits::kFragmentStage;

    TextureBindingLayout textureLayout{};
    textureLayout.index = 1;
    textureLayout.stages = BindingStageFlagBits::kFragmentStage;

    BindGroupLayoutDescriptor descriptor{};
    descriptor.samplers = { samplerLayout };
    descriptor.textures = { textureLayout };

    m_onscreen.bindGroupLayout = m_device->createBindGroupLayout(descriptor);
}

void OffscreenSample::createOnscreenBindGroup()
{
    SamplerBinding samplerBinding{
        .index = 0,
        .sampler = m_onscreen.sampler.get()
    };

    TextureBinding textureBinding{
        .index = 1,
        .textureView = m_offscreen.renderTextureView.get()
    };

    BindGroupDescriptor descriptor{
        .layout = m_onscreen.bindGroupLayout.get(),
        .samplers = { samplerBinding },
        .textures = { textureBinding }
    };

    m_onscreen.bindGroup = m_device->createBindGroup(descriptor);
}

void OffscreenSample::createOnscreenRenderPipeline()
{
    // render pipeline layout
    {
        PipelineLayoutDescriptor descriptor{};
        descriptor.layouts = { m_onscreen.bindGroupLayout.get() };

        m_onscreen.renderPipelineLayout = m_device->createPipelineLayout(descriptor);
    }

    // input assembly stage
    InputAssemblyStage inputAssemblyStage{};
    {
        inputAssemblyStage.topology = PrimitiveTopology::kTriangleList;
    }

    // vertex shader module
    std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "onscreen.vert.spv", m_handle);
        descriptor.code = vertexShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(vertexShaderSource.size());

        vertexShaderModule = m_device->createShaderModule(descriptor);
    }

    // vertex stage

    VertexAttribute positionAttribute{};
    positionAttribute.format = VertexFormat::kFloat32x3;
    positionAttribute.offset = offsetof(OnscreenVertex, pos);
    positionAttribute.location = 0;

    VertexAttribute texCoordAttribute{};
    texCoordAttribute.format = VertexFormat::kFloat32x2;
    texCoordAttribute.offset = offsetof(OnscreenVertex, texCoord);
    texCoordAttribute.location = 1;

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(OnscreenVertex);
    vertexInputLayout.attributes = { positionAttribute, texCoordAttribute };

    VertexStage vertexStage{
        { vertexShaderModule.get(), "main" },
        { vertexInputLayout }
    };

    // rasterization
    RasterizationStage rasterizationStage{};
    {
        rasterizationStage.cullMode = CullMode::kNone;
        rasterizationStage.frontFace = FrontFace::kCounterClockwise;
        rasterizationStage.sampleCount = m_sampleCount;
    }

    // fragment shader module
    std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "onscreen.frag.spv", m_handle);
        descriptor.code = fragmentShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(fragmentShaderSource.size());

        fragmentShaderModule = m_device->createShaderModule(descriptor);
    }

    // fragment

    FragmentStage::Target target{};
    target.format = m_swapchain->getTextureFormat();

    FragmentStage fragmentStage{
        { fragmentShaderModule.get(), "main" },
        { target }
    };

    // depth/stencil

    // render pipeline
    RenderPipelineDescriptor descriptor{
        m_onscreen.renderPipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    m_onscreen.renderPipeline = m_device->createRenderPipeline(descriptor);
}

} // namespace jipu
