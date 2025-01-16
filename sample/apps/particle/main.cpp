
#include "file.h"
#include "native_sample.h"

#include "jipu/native/adapter.h"
#include "jipu/native/buffer.h"
#include "jipu/native/device.h"
#include "jipu/native/physical_device.h"
#include "jipu/native/shader_module.h"
#include "jipu/native/surface.h"
#include "jipu/native/swapchain.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <glm/glm.hpp>
#include <random>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

namespace
{

uint64_t getCurrentTime()
{
    return duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace

class ParticleSample : public NativeSample
{
public:
    ParticleSample() = delete;
    ParticleSample(const SampleDescriptor& descriptor);
    ~ParticleSample() override;

public:
    void init() override;
    void onUpdate() override;
    void onDraw() override;

private:
    void updateImGui();

private:
    void createUniformBuffer();
    void createShaderStorageBuffer();
    void createColorAttachmentTexture();
    void createColorAttachmentTextureView();
    void createComputeBindGroup();
    void createComputePipeline();
    void createRenderPipeline();

    void updateUniformBuffer();

private:
    struct Particle
    {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::vec4 color;
    };

    std::unique_ptr<BindGroupLayout> m_computeBindGroupLayout = nullptr;
    std::vector<std::unique_ptr<BindGroup>> m_computeBindGroups{};
    std::unique_ptr<BindGroupLayout> m_renderBindGroupLayout = nullptr;
    std::unique_ptr<BindGroup> m_renderBindGroup = nullptr;

    std::unique_ptr<ShaderModule> m_vertexShaderModule = nullptr;
    std::unique_ptr<ShaderModule> m_fragmentShaderModule = nullptr;
    std::unique_ptr<PipelineLayout> m_computePipelineLayout = nullptr;
    std::unique_ptr<ComputePipeline> m_computePipeline = nullptr;
    std::unique_ptr<PipelineLayout> m_renderPipelineLayout = nullptr;
    std::unique_ptr<RenderPipeline> m_renderPipeline = nullptr;

    std::unique_ptr<Texture> m_colorAttachmentTexture = nullptr;
    std::unique_ptr<TextureView> m_colorAttachmentTextureView = nullptr;

    std::vector<std::unique_ptr<Buffer>> m_vertexBuffers{};
    std::unique_ptr<Buffer> m_uniformBuffer = nullptr;

    std::vector<Particle> m_vertices{};

    void* m_uniformBufferMappedPointer = nullptr;
    uint32_t m_sampleCount = 1; // use only 1, because there is not resolve texture.
    uint32_t m_particleCount = 8192;
    uint64_t m_previousTime = 0;
    uint64_t m_vertexIndex = 0;

private:
    bool separateCmdBuffer = false;
};

ParticleSample::ParticleSample(const SampleDescriptor& descriptor)
    : NativeSample(descriptor)
{
}

ParticleSample::~ParticleSample()
{
    m_renderPipeline.reset();
    m_renderPipelineLayout.reset();

    m_computePipeline.reset();
    m_computePipelineLayout.reset();

    m_fragmentShaderModule.reset();
    m_vertexShaderModule.reset();

    m_computeBindGroups.clear();
    m_computeBindGroupLayout.reset();

    m_colorAttachmentTextureView.reset();
    m_colorAttachmentTexture.reset();
    m_uniformBuffer.reset();
    m_vertexBuffers.clear();
}

void ParticleSample::init()
{
    NativeSample::init();

    createHPCWatcher();

    createShaderStorageBuffer();
    createUniformBuffer();
    createColorAttachmentTexture();
    createColorAttachmentTextureView();

    createComputeBindGroup();

    createComputePipeline();
    createRenderPipeline();
}

void ParticleSample::onUpdate()
{
    NativeSample::onUpdate();

    updateUniformBuffer();

    updateImGui();
}

void ParticleSample::updateImGui()
{
    recordImGui({ [&]() {
        windowImGui("Settings", { [&]() {
                        ImGui::Checkbox("Separate Command Buffer", &separateCmdBuffer);
                    } });
        profilingWindow();
    } });
}

void ParticleSample::onDraw()
{
    // encoder compute command
    std::unique_ptr<CommandEncoder> computeCommandEncoder = m_device->createCommandEncoder(CommandEncoderDescriptor{});

    ComputePassEncoderDescriptor computePassDescriptor{};
    std::unique_ptr<ComputePassEncoder> computePassEncoder = computeCommandEncoder->beginComputePass(computePassDescriptor);
    computePassEncoder->setPipeline(m_computePipeline.get());
    computePassEncoder->setBindGroup(0, m_computeBindGroups[(m_vertexIndex + 1) % 2].get());
    computePassEncoder->dispatch(m_particleCount / 256, 1, 1);
    computePassEncoder->end();

    std::unique_ptr<CommandBuffer> computeCommandBuffer = computeCommandEncoder->finish(CommandBufferDescriptor{});

    // encode render command
    auto renderView = m_swapchain->acquireNextTextureView();

    std::unique_ptr<CommandEncoder> renderCommandEncoder = m_device->createCommandEncoder(CommandEncoderDescriptor{});

    ColorAttachment colorAttachment{
        .renderView = renderView
    };
    colorAttachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
    colorAttachment.loadOp = LoadOp::kClear;
    colorAttachment.storeOp = StoreOp::kStore;

    RenderPassEncoderDescriptor renderPassDescriptor{
        .colorAttachments = { colorAttachment }
    };

    std::unique_ptr<RenderPassEncoder> renderPassEncoder = renderCommandEncoder->beginRenderPass(renderPassDescriptor);
    renderPassEncoder->setPipeline(m_renderPipeline.get());
    renderPassEncoder->setVertexBuffer(0, m_vertexBuffers[m_vertexIndex].get());
    renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1); // set viewport state.
    renderPassEncoder->setScissor(0, 0, m_width, m_height);        // set scissor state.
    renderPassEncoder->draw(static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
    renderPassEncoder->end();

    drawImGui(renderCommandEncoder.get(), renderView);
    std::unique_ptr<CommandBuffer> renderCommandBuffer = renderCommandEncoder->finish(CommandBufferDescriptor{});

    if (separateCmdBuffer)
    {
        m_queue->submit({ computeCommandBuffer.get() });
        m_queue->submit({ renderCommandBuffer.get() });
        m_swapchain->present();
    }
    else
    {
        m_queue->submit({ computeCommandBuffer.get(), renderCommandBuffer.get() });
        m_swapchain->present();
    }

    m_vertexIndex = (m_vertexIndex + 1) % 2;
}

void ParticleSample::createShaderStorageBuffer()
{
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    m_vertices.resize(m_particleCount);
    for (auto& particle : m_vertices)
    {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
        float x = r * cos(theta) * m_height / m_height;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
        particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    // create vertex buffer
    uint64_t vertexSize = static_cast<uint64_t>(sizeof(Particle) * m_vertices.size());
    BufferDescriptor vertexBufferDescriptor{};
    vertexBufferDescriptor.size = vertexSize;
    vertexBufferDescriptor.usage = BufferUsageFlagBits::kVertex | BufferUsageFlagBits::kStorage;
    {
        auto vertexBuffer = m_device->createBuffer(vertexBufferDescriptor);
        void* mappedPointer = vertexBuffer->map();
        memcpy(mappedPointer, m_vertices.data(), vertexSize);
        vertexBuffer->unmap();

        m_vertexBuffers.push_back(std::move(vertexBuffer));
    }

    {
        auto vertexBuffer = m_device->createBuffer(vertexBufferDescriptor);
        void* mappedPointer = vertexBuffer->map();
        memcpy(mappedPointer, m_vertices.data(), vertexSize);
        vertexBuffer->unmap();

        m_vertexBuffers.push_back(std::move(vertexBuffer));
    }
}

void ParticleSample::createUniformBuffer()
{
    const uint64_t uniforBufferSize = sizeof(float);

    BufferDescriptor descriptor{};
    descriptor.size = uniforBufferSize;
    descriptor.usage = BufferUsageFlagBits::kUniform;
    m_uniformBuffer = m_device->createBuffer(descriptor);

    m_uniformBufferMappedPointer = m_uniformBuffer->map();
}

void ParticleSample::createColorAttachmentTexture()
{
    TextureDescriptor descriptor{};
    descriptor.format = m_swapchain->getTextureFormat();
    descriptor.type = TextureType::k2D;
    descriptor.usage = TextureUsageFlagBits::kRenderAttachment;
    descriptor.width = m_swapchain->getWidth();
    descriptor.height = m_swapchain->getHeight();
    descriptor.depth = 1;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = m_sampleCount;

    m_colorAttachmentTexture = m_device->createTexture(descriptor);
}

void ParticleSample::createColorAttachmentTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_colorAttachmentTextureView = m_colorAttachmentTexture->createTextureView(descriptor);
}

void ParticleSample::createComputeBindGroup()
{
    BufferBindingLayout bufferUBOBindingLayout{};
    bufferUBOBindingLayout.index = 0;
    bufferUBOBindingLayout.stages = BindingStageFlagBits::kComputeStage;
    bufferUBOBindingLayout.type = BufferBindingType::kUniform;

    BufferBindingLayout bufferInBindingLayout{};
    bufferInBindingLayout.index = 1;
    bufferInBindingLayout.stages = BindingStageFlagBits::kVertexStage | BindingStageFlagBits::kComputeStage;
    bufferInBindingLayout.type = BufferBindingType::kStorage;

    BufferBindingLayout bufferOutBindingLayout{};
    bufferOutBindingLayout.index = 2;
    bufferOutBindingLayout.stages = BindingStageFlagBits::kVertexStage | BindingStageFlagBits::kComputeStage;
    bufferOutBindingLayout.type = BufferBindingType::kStorage;

    BindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.buffers = {
        bufferUBOBindingLayout,
        bufferInBindingLayout,
        bufferOutBindingLayout
    };
    m_computeBindGroupLayout = m_device->createBindGroupLayout(bindGroupLayoutDescriptor);

    {
        BufferBinding bufferUBOBinding{
            .index = 0,
            .offset = 0,
            .size = m_uniformBuffer->getSize(),
            .buffer = m_uniformBuffer.get(),
        };

        BufferBinding bufferInBinding{
            .index = 1,
            .offset = 0,
            .size = m_vertexBuffers[0]->getSize(),
            .buffer = m_vertexBuffers[0].get(),
        };

        BufferBinding bufferOutBinding{
            .index = 2,
            .offset = 0,
            .size = m_vertexBuffers[1]->getSize(),
            .buffer = m_vertexBuffers[1].get(),
        };

        BindGroupDescriptor bindGroupDescriptor{
            .layout = m_computeBindGroupLayout.get(),
            .buffers = {
                bufferUBOBinding,
                bufferInBinding,
                bufferOutBinding,
            },
            .samplers = {},
            .textures = {},
        };

        auto computeBindGroup = m_device->createBindGroup(bindGroupDescriptor);
        m_computeBindGroups.push_back(std::move(computeBindGroup));
    }

    {
        BufferBinding bufferUBOBinding{
            .index = 0,
            .offset = 0,
            .size = m_uniformBuffer->getSize(),
            .buffer = m_uniformBuffer.get(),
        };

        BufferBinding bufferInBinding{
            .index = 1,
            .offset = 0,
            .size = m_vertexBuffers[1]->getSize(),
            .buffer = m_vertexBuffers[1].get(),
        };

        BufferBinding bufferOutBinding{
            .index = 2,
            .offset = 0,
            .size = m_vertexBuffers[0]->getSize(),
            .buffer = m_vertexBuffers[0].get(),
        };

        BindGroupDescriptor bindGroupDescriptor{
            .layout = m_computeBindGroupLayout.get(),
            .buffers = {
                bufferUBOBinding,
                bufferInBinding,
                bufferOutBinding,
            },
            .samplers = {},
            .textures = {},
        };

        auto computeBindGroup = m_device->createBindGroup(bindGroupDescriptor);
        m_computeBindGroups.push_back(std::move(computeBindGroup));
    }
}

void ParticleSample::createComputePipeline()
{
    // pipeline layout
    PipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.layouts = { m_computeBindGroupLayout.get() };
    m_computePipelineLayout = m_device->createPipelineLayout(pipelineLayoutDescriptor);

    // compute shader
    const std::vector<char> computeShaderSource = utils::readFile(m_appDir / "particle.comp.spv", m_handle);
    ShaderModuleDescriptor shaderModuleDescriptor{ .type = ShaderModuleType::kSPIRV,
                                                   .code = std::string_view(computeShaderSource.data(), computeShaderSource.size()) };
    auto computeShader = m_device->createShaderModule(shaderModuleDescriptor);
    ComputeStage computeStage{
        { computeShader.get(), "main" }
    };

    ComputePipelineDescriptor computePipelineDescriptor{
        m_computePipelineLayout.get(),
        computeStage
    };

    m_computePipeline = m_device->createComputePipeline(computePipelineDescriptor);
}

void ParticleSample::createRenderPipeline()
{
    // pipeline layout
    PipelineLayoutDescriptor pipelineLayoutdescriptor{};
    pipelineLayoutdescriptor.layouts = {};
    m_renderPipelineLayout = m_device->createPipelineLayout(pipelineLayoutdescriptor);

    // input assembly
    InputAssemblyStage inputAssembly{};
    inputAssembly.topology = PrimitiveTopology::kPointList;

    // vertex shader

    // create vertex shader module.
    {
        const std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "particle.vert.spv", m_handle);
        ShaderModuleDescriptor shaderModuleDescriptor{};
        shaderModuleDescriptor.type = ShaderModuleType::kSPIRV;
        shaderModuleDescriptor.code = std::string_view(vertexShaderSource.data(), static_cast<uint32_t>(vertexShaderSource.size()));
        m_vertexShaderModule = m_device->createShaderModule(shaderModuleDescriptor);
    }

    // create vertex shader layouts

    std::vector<VertexAttribute> vertexAttributes{};
    vertexAttributes.resize(2);
    {
        // position
        vertexAttributes[0] = { .format = VertexFormat::kFloat32x3,
                                .offset = offsetof(Particle, position),
                                .location = 0 };

        // texture coodinate
        vertexAttributes[1] = { .format = VertexFormat::kFloat32x4,
                                .offset = offsetof(Particle, color),
                                .location = 1 };
    }

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.attributes = vertexAttributes;
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(Particle);

    VertexStage vertexStage{
        { m_vertexShaderModule.get(), "main" },
        { vertexInputLayout }
    };

    // rasterization
    RasterizationStage rasterizationStage{};
    {
        rasterizationStage.sampleCount = m_sampleCount;
        rasterizationStage.cullMode = CullMode::kBack;
        rasterizationStage.frontFace = FrontFace::kCounterClockwise;
    }

    // fragment shader

    const std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "particle.frag.spv", m_handle);
    ShaderModuleDescriptor shaderModuleDescriptor{};
    shaderModuleDescriptor.type = ShaderModuleType::kSPIRV;
    shaderModuleDescriptor.code = std::string_view(fragmentShaderSource.data(), static_cast<uint32_t>(fragmentShaderSource.size()));
    m_fragmentShaderModule = m_device->createShaderModule(shaderModuleDescriptor);

    FragmentStage::Target target{};
    target.format = m_swapchain->getTextureFormat();

    FragmentStage fragmentStage{
        { m_fragmentShaderModule.get(), "main" },
        { target }
    };

    RenderPipelineDescriptor renderPipelineDescriptor{
        m_renderPipelineLayout.get(),
        inputAssembly,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    m_renderPipeline = m_device->createRenderPipeline(renderPipelineDescriptor);
}

void ParticleSample::updateUniformBuffer()
{
    if (m_previousTime == 0)
    {
        m_previousTime = getCurrentTime();
    }

    uint64_t currentTime = getCurrentTime();

    float deltaTime = (currentTime - m_previousTime) * 10.0f;
    memcpy(m_uniformBufferMappedPointer, &deltaTime, sizeof(deltaTime));

    m_previousTime = currentTime;
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
        { 1000, 2000, "Particle Sample", app },
        ""
    };

    jipu::ParticleSample sample(descriptor);

    sample.exec();
}

#else

int main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::trace);

    jipu::SampleDescriptor descriptor{
        { 800, 600, "Particle Sample", nullptr },
        argv[0]
    };

    jipu::ParticleSample sample(descriptor);

    return sample.exec();
}

#endif
