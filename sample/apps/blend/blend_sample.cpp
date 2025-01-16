#include "blend_sample.h"

namespace jipu
{

BlendSample::BlendSample(const SampleDescriptor& descriptor)
    : NativeSample(descriptor)
{
    // do not call init() function. it will be called in window exec() function.
}

BlendSample::~BlendSample()
{
    m_sampler.reset();
    m_textureView1.reset();
    m_textureView2.reset();
    m_texture1.reset();
    m_texture2.reset();
    m_renderPipeline1.reset();
    m_renderPipeline2.reset();
    m_renderPipelineLayout.reset();
    m_bindGroup1.reset();
    m_bindGroup2.reset();
    m_bindGroupLayout.reset();
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
}

void BlendSample::init()
{
    NativeSample::init();

    createHPCWatcher();

    createVertexBuffer();
    createIndexBuffer();
    m_texture1 = createTexture("viking_room.png");
    m_texture2 = createTexture("rgb.png");
    m_textureView1 = createTextureView(m_texture1.get());
    m_textureView2 = createTextureView(m_texture2.get());
    createSampler();
    createBindGroupLayout();
    m_bindGroup1 = createBindGroup(m_textureView1.get());
    m_bindGroup2 = createBindGroup(m_textureView2.get());
    createRenderPipelineLayout();
    m_renderPipeline1 = createRenderPipeline(BlendState{}); // default blend state.
    m_renderPipeline2 = createRenderPipeline(BlendState{}); // default blend state.
}

void BlendSample::onUpdate()
{
    NativeSample::onUpdate();

    updateImGui();
}

void BlendSample::onDraw()
{
    auto renderView = m_swapchain->acquireNextTextureView();
    {
        ColorAttachment attachment{
            .renderView = renderView
        };
        attachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        attachment.loadOp = LoadOp::kClear;
        attachment.storeOp = StoreOp::kStore;

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { attachment },
            .occlusionQuerySet = nullptr,
            .timestampWrites = {}
        };

        CommandEncoderDescriptor commandDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandDescriptor);

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_renderPipeline1.get());
        renderPassEncoder->setBindGroup(0, m_bindGroup1.get());
        renderPassEncoder->setVertexBuffer(0, m_vertexBuffer.get());
        renderPassEncoder->setIndexBuffer(m_indexBuffer.get(), IndexFormat::kUint16);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        renderPassEncoder->setPipeline(m_renderPipeline2.get());
        renderPassEncoder->setBindGroup(0, m_bindGroup2.get());
        renderPassEncoder->setBlendConstant({ 0.5, 0.5, 0.5, 0.0 });
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        renderPassEncoder->end();

        drawImGui(commandEncoder.get(), renderView);

        auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
        m_queue->submit({ commandBuffer.get() });
        m_swapchain->present();
    }
}

void BlendSample::updateImGui()
{
    static const std::vector<std::string> opItems = { "kAdd", "kSubtract", "kReversSubtract", "kMin", "kMax" };
    static const std::vector<std::string> factorItems = { "kZero", "kOne", "kSrcColor", "kSrcAlpha", "kSrcAlphaSaturated",
                                                          "kOneMinusSrcColor", "kOneMinusSrcAlpha", "kDstColor", "kDstAlpha", "kOneMinusDstColor",
                                                          "kOneMinusDstAlpha", "kConstantColor", "kOneMinusConstantColor",
                                                          "kSrc1Color", "kOneMinusSrc1Color", "kSrc1Alpha", "kOneMinusSrc1Alpha" };
    auto combobox = [&](const char* label, const std::vector<std::string>& items, uint32_t* selected) {
        if (ImGui::BeginCombo(label, items[*selected].c_str()))
        {
            for (uint32_t i = 0; i < items.size(); ++i)
            {
                bool isSelected = *selected == i;
                if (ImGui::Selectable(items[i].c_str(), isSelected))
                {
                    *selected = i;
                    m_renderPipeline2 = createRenderPipeline(BlendState{
                        .color = BlendComponent{
                            .srcFactor = static_cast<BlendFactor>(m_blendColorSrcFactor),
                            .dstFactor = static_cast<BlendFactor>(m_blendColorDstFactor),
                            .operation = static_cast<BlendOperation>(m_blendColorOp),
                        },
                        .alpha = BlendComponent{
                            .srcFactor = static_cast<BlendFactor>(m_blendAlphaSrcFactor),
                            .dstFactor = static_cast<BlendFactor>(m_blendAlphaDstFactor),
                            .operation = static_cast<BlendOperation>(m_blendAlphaOp),
                        } });
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    };

    recordImGui({ [&]() {
        windowImGui("Color Blend",
                    { [&]() {
                        combobox("Op", opItems, &m_blendColorOp);
                        combobox("Src Factor", factorItems, &m_blendColorSrcFactor);
                        combobox("Dst Factor", factorItems, &m_blendColorDstFactor);
                    } });
        windowImGui("Alpha Blend",
                    { [&]() {
                        combobox("Op", opItems, &m_blendAlphaOp);
                        combobox("Src Factor", factorItems, &m_blendAlphaSrcFactor);
                        combobox("Dst Factor", factorItems, &m_blendAlphaDstFactor);
                    } });
        profilingWindow();
    } });
}

void BlendSample::createVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_vertices.size() * sizeof(Vertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_vertexBuffer->map();
    memcpy(pointer, m_vertices.data(), descriptor.size);
    m_vertexBuffer->unmap();
}

void BlendSample::createIndexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_indices.size() * sizeof(uint16_t);
    descriptor.usage = BufferUsageFlagBits::kIndex;

    m_indexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_indexBuffer->map();
    memcpy(pointer, m_indices.data(), descriptor.size);
    m_indexBuffer->unmap();
}

void BlendSample::createSampler()
{
    SamplerDescriptor samplerDescriptor{};
    samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
    samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
    samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
    samplerDescriptor.lodMin = 0.0f;
    samplerDescriptor.lodMax = static_cast<float>(m_textureView1->getTexture()->getMipLevels());
    samplerDescriptor.minFilter = FilterMode::kLinear;
    samplerDescriptor.magFilter = FilterMode::kLinear;
    samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;

    m_sampler = m_device->createSampler(samplerDescriptor);
}

std::unique_ptr<Texture> BlendSample::createTexture(const char* name)
{
    std::vector<char> buffer = utils::readFile(m_appDir / name, m_handle);
    auto image = std::make_unique<Image>(buffer.data(), buffer.size());

    unsigned char* pixels = static_cast<unsigned char*>(image->getPixels());
    uint32_t width = image->getWidth();
    uint32_t height = image->getHeight();
    uint32_t channel = image->getChannel();
    uint64_t imageSize = sizeof(unsigned char) * width * height * channel;

    // create image staging buffer.
    BufferDescriptor descriptor{ .size = imageSize, .usage = BufferUsageFlagBits::kCopySrc };
    std::unique_ptr<Buffer> imageTextureStagingBuffer = m_device->createBuffer(descriptor);

    void* mappedPointer = imageTextureStagingBuffer->map();
    memcpy(mappedPointer, pixels, imageSize);

    // create texture.
    TextureDescriptor textureDescriptor{ .type = TextureType::k2D,
                                         .format = TextureFormat::kRGBA8UnormSrgb,
                                         .usage = TextureUsageFlagBits::kCopySrc |
                                                  TextureUsageFlagBits::kCopyDst |
                                                  TextureUsageFlagBits::kTextureBinding,
                                         .width = width,
                                         .height = height,
                                         .depth = 1,
                                         .mipLevels = 1, // static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
                                         .sampleCount = 1 };
    auto texture = m_device->createTexture(textureDescriptor);

    // copy image staging buffer to texture
    copyBufferToTexture(*imageTextureStagingBuffer, *texture);

    return texture;
}

std::unique_ptr<TextureView> BlendSample::createTextureView(Texture* texture)
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    return texture->createTextureView(descriptor);
}

void BlendSample::createBindGroupLayout()
{
    SamplerBindingLayout imageSamplerBindingLayout{};
    imageSamplerBindingLayout.index = 0;
    imageSamplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    TextureBindingLayout imageTextureBindingLayout{};
    imageTextureBindingLayout.index = 1;
    imageTextureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    BindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.samplers = { imageSamplerBindingLayout };
    bindGroupLayoutDescriptor.textures = { imageTextureBindingLayout };

    m_bindGroupLayout = m_device->createBindGroupLayout(bindGroupLayoutDescriptor);
}

std::unique_ptr<BindGroup> BlendSample::createBindGroup(TextureView* textureView)
{
    SamplerBinding imageSamplerBinding{
        .index = 0,
        .sampler = m_sampler.get(),
    };

    TextureBinding imageTextureBinding{
        .index = 1,
        .textureView = textureView,
    };
    BindGroupDescriptor bindGroupDescriptor{
        .layout = m_bindGroupLayout.get(),
        .samplers = { imageSamplerBinding },
        .textures = { imageTextureBinding },
    };

    return m_device->createBindGroup(bindGroupDescriptor);
}

void BlendSample::createRenderPipelineLayout()
{
    // render pipeline layout
    {
        PipelineLayoutDescriptor descriptor{};
        descriptor.layouts = { m_bindGroupLayout.get() };

        m_renderPipelineLayout = m_device->createPipelineLayout(descriptor);
    }
}

std::unique_ptr<RenderPipeline> BlendSample::createRenderPipeline(const BlendState& blendState)
{
    // input assembly stage
    InputAssemblyStage inputAssemblyStage{};
    {
        inputAssemblyStage.topology = PrimitiveTopology::kTriangleList;
    }

    // vertex shader module
    std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "quad.vert.spv", m_handle);
        descriptor.type = ShaderModuleType::kSPIRV;
        descriptor.code = std::string_view(vertexShaderSource.data(), static_cast<uint32_t>(vertexShaderSource.size()));

        vertexShaderModule = m_device->createShaderModule(descriptor);
    }

    // vertex stage

    VertexAttribute positionAttribute{};
    positionAttribute.format = VertexFormat::kFloat32x2;
    positionAttribute.offset = offsetof(Vertex, pos);
    positionAttribute.location = 0;

    VertexAttribute texCoordAttribute{};
    texCoordAttribute.format = VertexFormat::kFloat32x2;
    texCoordAttribute.offset = offsetof(Vertex, texCoord);
    texCoordAttribute.location = 1;

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(Vertex);
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
        std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "quad.frag.spv", m_handle);
        descriptor.type = ShaderModuleType::kSPIRV;
        descriptor.code = std::string_view(fragmentShaderSource.data(), static_cast<uint32_t>(fragmentShaderSource.size()));

        fragmentShaderModule = m_device->createShaderModule(descriptor);
    }

    // fragment
    FragmentStage::Target target{};
    target.format = m_swapchain->getTextureFormat();
    target.blend = blendState;

    FragmentStage fragmentStage{
        { fragmentShaderModule.get(), "main" },
        { target }
    };

    // depth/stencil

    // render pipeline
    RenderPipelineDescriptor descriptor{
        m_renderPipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    return m_device->createRenderPipeline(descriptor);
}

void BlendSample::copyBufferToTexture(Buffer& imageTextureStagingBuffer, Texture& imageTexture)
{
    CopyTextureBuffer copyTextureBuffer{
        .buffer = &imageTextureStagingBuffer,
        .offset = 0,
        .bytesPerRow = 0,
        .rowsPerTexture = 0
    };

    uint32_t channel = 4;                          // TODO: from texture.
    uint32_t bytesPerData = sizeof(unsigned char); // TODO: from buffer.
    copyTextureBuffer.bytesPerRow = bytesPerData * imageTexture.getWidth() * channel;
    copyTextureBuffer.rowsPerTexture = imageTexture.getHeight();

    CopyTexture copyTexture{ .texture = &imageTexture, .aspect = TextureAspectFlagBits::kColor };
    Extent3D extent{};
    extent.width = imageTexture.getWidth();
    extent.height = imageTexture.getHeight();
    extent.depth = 1;

    CommandEncoderDescriptor commandEncoderDescriptor{};
    std::unique_ptr<CommandEncoder> commandEndoer = m_device->createCommandEncoder(commandEncoderDescriptor);
    commandEndoer->copyBufferToTexture(copyTextureBuffer, copyTexture, extent);

    auto commandBuffer = commandEndoer->finish(CommandBufferDescriptor{});
    m_queue->submit({ commandBuffer.get() });
}

} // namespace jipu