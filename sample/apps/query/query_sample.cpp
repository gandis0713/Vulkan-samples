#include "query_sample.h"

namespace jipu
{

QuerySample::QuerySample(const SampleDescriptor& descriptor)
    : NativeSample(descriptor)
{
    // do not call init() function. it will be called in window exec() function.
}

QuerySample::~QuerySample()
{
    m_timestampQueryBuffer.reset();
    m_occlusionQueryBuffer.reset();
    m_timestampQuerySet.reset();
    m_occlusionQuerySet.reset();
    m_renderPipeline.reset();
    m_renderPipelineLayout.reset();
    m_bindGroup.reset();
    m_bindGroupLayout.reset();
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
    m_uniformBuffer.reset();
}

void QuerySample::init()
{
    NativeSample::init();

    createHPCWatcher();

    createCamera(); // need size and aspect ratio from swapchain.

    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createBindGroupLayout();
    createBindGroup();
    createRenderPipeline();
    createQuerySet();
}

void QuerySample::createCamera()
{
    m_camera = std::make_unique<PerspectiveCamera>(45.0f,
                                                   m_swapchain->getWidth() / static_cast<float>(m_swapchain->getHeight()),
                                                   0.1f,
                                                   1000.0f);

    // auto halfWidth = m_swapchain->getWidth() / 2.0f;
    // auto halfHeight = m_swapchain->getHeight() / 2.0f;
    // m_camera = std::make_unique<OrthographicCamera>(-halfWidth, halfWidth,
    //                                                 -halfHeight, halfHeight,
    //                                                 -1000, 1000);

    m_camera->lookAt(glm::vec3(0.0f, 0.0f, 1000.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0f, 0.0));
}

void QuerySample::updateUniformBuffer()
{
    m_ubo.mvp.model = glm::mat4(1.0f);
    m_ubo.mvp.view = m_camera->getViewMat();
    m_ubo.mvp.proj = m_camera->getProjectionMat();

    void* pointer = m_uniformBuffer->map(); // do not unmap.
    memcpy(pointer, &m_ubo, m_uniformBuffer->getSize());
}

void QuerySample::onUpdate()
{
    NativeSample::onUpdate();

    updateUniformBuffer();

    updateImGui();
}

void QuerySample::onDraw()
{
    auto renderView = m_swapchain->acquireNextTextureView();
    {
        ColorAttachment attachment{
            .renderView = renderView
        };
        attachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        attachment.loadOp = LoadOp::kClear;
        attachment.storeOp = StoreOp::kStore;

        RenderPassTimestampWrites timestampWrites;

        if (m_useTimestamp)
        {
            timestampWrites.querySet = m_timestampQuerySet.get();
            timestampWrites.beginQueryIndex = 0;
            timestampWrites.endQueryIndex = 1;
        }

        QuerySet* occlusionQuerySet = nullptr;
        if (m_useOcclusion)
        {
            occlusionQuerySet = m_occlusionQuerySet.get();
        }

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { attachment },
            .occlusionQuerySet = occlusionQuerySet,
            .timestampWrites = timestampWrites
        };

        CommandEncoderDescriptor commandDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandDescriptor);

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        if (m_useOcclusion)
        {
            renderPassEncoder->beginOcclusionQuery(0);
        }
        renderPassEncoder->setPipeline(m_renderPipeline.get());
        renderPassEncoder->setBindGroup(0, m_bindGroup.get());
        renderPassEncoder->setVertexBuffer(0, m_vertexBuffer.get());
        renderPassEncoder->setIndexBuffer(m_indexBuffer.get(), IndexFormat::kUint16);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        if (m_useOcclusion)
        {
            renderPassEncoder->endOcclusionQuery();
        }
        renderPassEncoder->end();

        drawImGui(commandEncoder.get(), renderView);

        if (m_useTimestamp)
        {
            commandEncoder->resolveQuerySet(m_timestampQuerySet.get(),
                                            0,
                                            m_timestampQuerySet->getCount(),
                                            m_timestampQueryBuffer.get(),
                                            0);
        }

        if (m_useOcclusion)
        {
            commandEncoder->resolveQuerySet(m_occlusionQuerySet.get(),
                                            0,
                                            m_occlusionQuerySet->getCount(),
                                            m_occlusionQueryBuffer.get(),
                                            0);
        }

        auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
        m_queue->submit({ commandBuffer.get() });
        m_swapchain->present();

        if (m_useTimestamp)
        {
            static uint32_t count = 0;
            static double ms = 0;

            auto pointer = reinterpret_cast<uint64_t*>(m_timestampQueryBuffer->map());
            uint64_t elapsedTime = pointer[1] - pointer[0]; // nano seconds
            double miliElapsedTime = elapsedTime / 1000.0 / 1000.0;
            ms += miliElapsedTime;
            spdlog::debug("pipeline elapsed time [Avg {:.3f},  Cur {:.3f}]", (ms / count), miliElapsedTime);
            ++count;
        }

        if (m_useOcclusion)
        {
            auto pointer = reinterpret_cast<uint64_t*>(m_occlusionQueryBuffer->map());
            spdlog::debug("fragment sample count [{}]", pointer[0]);
        }
    }
}

void QuerySample::updateImGui()
{
    recordImGui({ [&]() {
        windowImGui("Query", { [&]() {
                        ImGui::Checkbox("Log Timestamp", &m_useTimestamp);
                        ImGui::Checkbox("Log Occlusion", &m_useOcclusion);
                    } });
        profilingWindow();
    } });
}

void QuerySample::createVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_vertices.size() * sizeof(Vertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_vertexBuffer->map();
    memcpy(pointer, m_vertices.data(), descriptor.size);
    m_vertexBuffer->unmap();
}

void QuerySample::createIndexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_indices.size() * sizeof(uint16_t);
    descriptor.usage = BufferUsageFlagBits::kIndex;

    m_indexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_indexBuffer->map();
    memcpy(pointer, m_indices.data(), descriptor.size);
    m_indexBuffer->unmap();
}

void QuerySample::createUniformBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = sizeof(UBO);
    descriptor.usage = BufferUsageFlagBits::kUniform;

    m_uniformBuffer = m_device->createBuffer(descriptor);

    [[maybe_unused]] auto pointer = m_uniformBuffer->map();
    // memcpy(pointer, &m_ubo, descriptor.size);
    // m_uniformBuffer->unmap();
}

void QuerySample::createBindGroupLayout()
{
    BufferBindingLayout bufferLayout{};
    bufferLayout.index = 0;
    bufferLayout.stages = BindingStageFlagBits::kVertexStage;
    bufferLayout.type = BufferBindingType::kUniform;

    BindGroupLayoutDescriptor descriptor{};
    descriptor.buffers = { bufferLayout };

    m_bindGroupLayout = m_device->createBindGroupLayout(descriptor);
}

void QuerySample::createBindGroup()
{
    BufferBinding bufferBinding{
        .index = 0,
        .offset = 0,
        .size = m_uniformBuffer->getSize(),
        .buffer = m_uniformBuffer.get(),
    };

    BindGroupDescriptor descriptor{
        .layout = m_bindGroupLayout.get(),
        .buffers = { bufferBinding },
    };

    m_bindGroup = m_device->createBindGroup(descriptor);
}

void QuerySample::createRenderPipeline()
{
    // render pipeline layout
    {
        PipelineLayoutDescriptor descriptor{};
        descriptor.layouts = { m_bindGroupLayout.get() };

        m_renderPipelineLayout = m_device->createPipelineLayout(descriptor);
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
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "triangle.vert.spv", m_handle);
        descriptor.type = ShaderModuleType::kSPIRV;
        descriptor.code = std::string_view(vertexShaderSource.data(), vertexShaderSource.size());

        vertexShaderModule = m_device->createShaderModule(descriptor);
    }

    // vertex stage

    VertexAttribute positionAttribute{};
    positionAttribute.format = VertexFormat::kFloat32x3;
    positionAttribute.offset = offsetof(Vertex, pos);
    positionAttribute.location = 0;

    VertexAttribute colorAttribute{};
    colorAttribute.format = VertexFormat::kFloat32x3;
    colorAttribute.offset = offsetof(Vertex, color);
    colorAttribute.location = 1;

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(Vertex);
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
        std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "triangle.frag.spv", m_handle);
        descriptor.type = ShaderModuleType::kSPIRV;
        descriptor.code = std::string_view(fragmentShaderSource.data(), fragmentShaderSource.size());

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
        m_renderPipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    m_renderPipeline = m_device->createRenderPipeline(descriptor);
}

void QuerySample::createQuerySet()
{
    {
        QuerySetDescriptor timestampQuerySetDescriptor{
            .type = QueryType::kTimestamp,
            .count = 2
        };

        m_timestampQuerySet = m_device->createQuerySet(timestampQuerySetDescriptor);

        BufferDescriptor timestampBufferDescriptor{};
        timestampBufferDescriptor.size = timestampQuerySetDescriptor.count * sizeof(uint64_t);
        timestampBufferDescriptor.usage = BufferUsageFlagBits::kQueryResolve | BufferUsageFlagBits::kMapRead;

        m_timestampQueryBuffer = m_device->createBuffer(timestampBufferDescriptor);
    }

    {
        QuerySetDescriptor occlusionQuerySetDescriptor{
            .type = QueryType::kOcclusion,
            .count = 1
        };

        m_occlusionQuerySet = m_device->createQuerySet(occlusionQuerySetDescriptor);

        BufferDescriptor occlusionBufferDescriptor{};
        occlusionBufferDescriptor.size = occlusionQuerySetDescriptor.count * sizeof(uint64_t);
        occlusionBufferDescriptor.usage = BufferUsageFlagBits::kQueryResolve | BufferUsageFlagBits::kMapRead;

        m_occlusionQueryBuffer = m_device->createBuffer(occlusionBufferDescriptor);
    }
}

} // namespace jipu