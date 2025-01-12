

#include "file.h"
#include "native_sample.h"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

class ImGuiSample : public NativeSample
{
public:
    ImGuiSample() = delete;
    ImGuiSample(const SampleDescriptor& descriptor);
    ~ImGuiSample() override;

    void init() override;
    void onUpdate() override;
    void onDraw() override;

private:
    void updateImGui();

private:
    void createVertexBuffer();
    void createRenderPipeline();

private:
    std::unique_ptr<Buffer> m_vertexBuffer = nullptr;
    std::unique_ptr<RenderPipeline> m_renderPipeline = nullptr;

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    std::vector<Vertex> m_vertices{
        { { 0.0, 0.5, 0.0 }, { 1.0, 0.0, 0.0 } },
        { { -0.5, -0.5, 0.0 }, { 0.0, 1.0, 0.0 } },
        { { 0.5, -0.5, 0.0 }, { 0.0, 0.0, 1.0 } },
    };

    uint32_t m_sampleCount = 1; // use only 1, because there is not resolve texture.
};

ImGuiSample::ImGuiSample(const SampleDescriptor& descriptor)
    : NativeSample(descriptor)
{
}

ImGuiSample::~ImGuiSample()
{
    m_renderPipeline.reset();
    m_vertexBuffer.reset();
}

void ImGuiSample::init()
{
    NativeSample::init();

    createHPCWatcher();

    createVertexBuffer();
    createRenderPipeline();
}

void ImGuiSample::onUpdate()
{
    NativeSample::onUpdate();

    updateImGui();
}

void ImGuiSample::onDraw()
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
            .colorAttachments = { attachment }
        };

        CommandEncoderDescriptor commandDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandDescriptor);

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);

        renderPassEncoder->setPipeline(m_renderPipeline.get());
        renderPassEncoder->setVertexBuffer(0, m_vertexBuffer.get());
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
        renderPassEncoder->end();

        drawImGui(commandEncoder.get(), renderView);

        auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
        m_queue->submit({ commandBuffer.get() });
        m_swapchain->present();
    }
}

void ImGuiSample::updateImGui()
{
    recordImGui({ [&]() {
        profilingWindow();
    } });
}

void ImGuiSample::createVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_vertices.size() * sizeof(Vertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_vertexBuffer->map();
    memcpy(pointer, m_vertices.data(), descriptor.size);
    m_vertexBuffer->unmap();
}

void ImGuiSample::createRenderPipeline()
{
    // render pipeline layout
    std::unique_ptr<PipelineLayout> renderPipelineLayout = nullptr;
    {
        PipelineLayoutDescriptor descriptor{};

        renderPipelineLayout = m_device->createPipelineLayout(descriptor);
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
        descriptor.code = vertexShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(vertexShaderSource.size());

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
        renderPipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    m_renderPipeline = m_device->createRenderPipeline(descriptor);
}

} // namespace jipu

#if defined(__ANDROID__) || defined(ANDROID)

// GameActivity's C/C++ code
#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

// // Glue from GameActivity to android_main()
// // Passing GameActivity event from main thread to app native thread.
extern "C"
{
#include <game-activity/native_app_glue/android_native_app_glue.c>
}

void android_main(struct android_app* app)
{
    jipu::SampleDescriptor descriptor{
        { 1000, 2000, "ImGui", app },
        ""
    };

    jipu::ImGuiSample sample(descriptor);

    sample.exec();
}

#else

int main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::trace);

    jipu::SampleDescriptor descriptor{
        { 800, 600, "ImGui", nullptr },
        argv[0]
    };

    jipu::ImGuiSample sample(descriptor);

    return sample.exec();
}

#endif
