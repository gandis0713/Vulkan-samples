

#include "camera.h"
#include "file.h"
#include "im_gui.h"
#include "sample.h"

#include "jipu/buffer.h"
#include "jipu/command_buffer.h"
#include "jipu/command_encoder.h"
#include "jipu/device.h"
#include "jipu/driver.h"
#include "jipu/physical_device.h"
#include "jipu/pipeline.h"
#include "jipu/pipeline_layout.h"
#include "jipu/queue.h"
#include "jipu/surface.h"
#include "jipu/swapchain.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

class TriangleSample : public Sample, public Im_Gui
{
public:
    TriangleSample() = delete;
    TriangleSample(const SampleDescriptor& descriptor);
    ~TriangleSample() override;

    void init() override;
    void update() override;
    void draw() override;

private:
    void updateImGui() override;

private:
    void createCamera();

    void updateUniformBuffer();

private:
    void createDevier();
    void getPhysicalDevices();
    void createSurface();
    void createDevice();
    void createSwapchain();
    void createCommandBuffer();
    void createQueue();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();
    void createBindingGroupLayout();
    void createBindingGroup();
    void createRenderPipeline();

private:
    std::unique_ptr<Driver> m_driver = nullptr;
    std::vector<std::unique_ptr<PhysicalDevice>> m_physicalDevices{};
    std::unique_ptr<Surface> m_surface = nullptr;
    std::unique_ptr<Device> m_device = nullptr;
    std::unique_ptr<Swapchain> m_swapchain = nullptr;
    std::unique_ptr<CommandBuffer> m_commandBuffer = nullptr;
    std::unique_ptr<Queue> m_queue = nullptr;
    std::unique_ptr<Buffer> m_vertexBuffer = nullptr;
    std::unique_ptr<Buffer> m_indexBuffer = nullptr;
    std::unique_ptr<Buffer> m_uniformBuffer = nullptr;
    std::unique_ptr<BindingGroupLayout> m_bindingGroupLayout = nullptr;
    std::unique_ptr<BindingGroup> m_bindingGroup = nullptr;
    std::unique_ptr<PipelineLayout> m_renderPipelineLayout = nullptr;
    std::unique_ptr<RenderPipeline> m_renderPipeline = nullptr;

    struct MVP
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct UBO
    {
        MVP mvp;
    } m_ubo;

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    std::vector<uint16_t> m_indices{ 0, 1, 2 };
    std::vector<Vertex>
        m_vertices{
            { { 0.0, -500, 0.0 }, { 1.0, 0.0, 0.0 } },
            { { -500, 500, 0.0 }, { 0.0, 1.0, 0.0 } },
            { { 500, 500, 0.0 }, { 0.0, 0.0, 1.0 } },
        };

    uint32_t m_sampleCount = 1;
    std::unique_ptr<Camera> m_camera = nullptr;
};

TriangleSample::TriangleSample(const SampleDescriptor& descriptor)
    : Sample(descriptor)
{
}

TriangleSample::~TriangleSample()
{
    clearImGui();

    m_renderPipeline.reset();
    m_renderPipelineLayout.reset();
    m_bindingGroup.reset();
    m_bindingGroupLayout.reset();
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
    m_uniformBuffer.reset();
    m_queue.reset();
    m_commandBuffer.reset();
    m_swapchain.reset();
    m_device.reset();
    m_surface.reset();
    m_physicalDevices.clear();
    m_driver.reset();
}

void TriangleSample::init()
{
    createDevier();
    getPhysicalDevices();
    createSurface();
    createDevice();
    createSwapchain();
    createCommandBuffer();
    createQueue();

    createCamera(); // need size and aspect ratio from swapchain.

    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createBindingGroupLayout();
    createBindingGroup();
    createRenderPipeline();

    initImGui(m_device.get(), m_queue.get(), m_swapchain.get());

    m_initialized = true;
}

void TriangleSample::createCamera()
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

void TriangleSample::updateUniformBuffer()
{
    m_ubo.mvp.model = glm::mat4(1.0f);
    m_ubo.mvp.view = m_camera->getViewMat();
    m_ubo.mvp.proj = m_camera->getProjectionMat();

    void* pointer = m_uniformBuffer->map(); // do not unmap.
    memcpy(pointer, &m_ubo, m_uniformBuffer->getSize());
}

void TriangleSample::update()
{
    updateUniformBuffer();

    updateImGui();
    buildImGui();
}

void TriangleSample::draw()
{
    auto renderView = m_swapchain->acquireNextTexture();
    {
        ColorAttachment attachment{
            .renderView = *renderView
        };
        attachment.clearValue = { .float32 = { 0.0, 0.0, 0.0, 0.0 } };
        attachment.loadOp = LoadOp::kClear;
        attachment.storeOp = StoreOp::kStore;

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { attachment },
            .sampleCount = m_sampleCount
        };

        CommandEncoderDescriptor commandDescriptor{};
        auto commadEncoder = m_commandBuffer->createCommandEncoder(commandDescriptor);

        auto renderPassEncoder = commadEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_renderPipeline.get());
        renderPassEncoder->setBindingGroup(0, m_bindingGroup.get());
        renderPassEncoder->setVertexBuffer(0, m_vertexBuffer.get());
        renderPassEncoder->setIndexBuffer(m_indexBuffer.get(), IndexFormat::kUint16);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
        renderPassEncoder->end();

        drawImGui(commadEncoder.get(), renderView);

        m_queue->submit({ commadEncoder->finish() }, m_swapchain.get());
    }
}

void TriangleSample::updateImGui()
{
    // set display size and mouse state.
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)m_width, (float)m_height);
        io.MousePos = ImVec2(m_mouseX, m_mouseY);
        io.MouseDown[0] = m_leftMouseButton;
        io.MouseDown[1] = m_rightMouseButton;
        io.MouseDown[2] = m_middleMouseButton;
    }

    ImGui::NewFrame();
    debugWindow();
    ImGui::Render();
}

void TriangleSample::createDevier()
{
    DriverDescriptor descriptor{};
    descriptor.type = DriverType::kVulkan;

    m_driver = Driver::create(descriptor);
}

void TriangleSample::getPhysicalDevices()
{
    m_physicalDevices = m_driver->getPhysicalDevices();
}

void TriangleSample::createDevice()
{
    // TODO: select suit device.
    PhysicalDevice* physicalDevice = m_physicalDevices[0].get();

    DeviceDescriptor descriptor;
    m_device = physicalDevice->createDevice(descriptor);
}

void TriangleSample::createSurface()
{
    SurfaceDescriptor descriptor{};
    descriptor.windowHandle = getWindowHandle();

    m_surface = m_driver->createSurface(descriptor);
}

void TriangleSample::createSwapchain()
{
#if defined(__ANDROID__) || defined(ANDROID)
    TextureFormat textureFormat = TextureFormat::kRGBA_8888_UInt_Norm_SRGB;
#else
    TextureFormat textureFormat = TextureFormat::kBGRA_8888_UInt_Norm_SRGB;
#endif

    SwapchainDescriptor descriptor{};
    descriptor.width = m_width;
    descriptor.height = m_height;
    descriptor.surface = m_surface.get();
    descriptor.textureFormat = textureFormat;
    descriptor.colorSpace = ColorSpace::kSRGBNonLinear;
    descriptor.presentMode = PresentMode::kFifo;

    m_swapchain = m_device->createSwapchain(descriptor);
}

void TriangleSample::createCommandBuffer()
{
    CommandBufferDescriptor descriptor{};
    descriptor.usage = CommandBufferUsage::kOneTime;

    m_commandBuffer = m_device->createCommandBuffer(descriptor);
}

void TriangleSample::createQueue()
{
    QueueDescriptor descriptor{};
    descriptor.flags = QueueFlagBits::kGraphics;

    m_queue = m_device->createQueue(descriptor);
}

void TriangleSample::createVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_vertices.size() * sizeof(Vertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_vertexBuffer->map();
    memcpy(pointer, m_vertices.data(), descriptor.size);
    m_vertexBuffer->unmap();
}

void TriangleSample::createIndexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_indices.size() * sizeof(uint16_t);
    descriptor.usage = BufferUsageFlagBits::kIndex;

    m_indexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_indexBuffer->map();
    memcpy(pointer, m_indices.data(), descriptor.size);
    m_indexBuffer->unmap();
}

void TriangleSample::createUniformBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = sizeof(UBO);
    descriptor.usage = BufferUsageFlagBits::kUniform;

    m_uniformBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_uniformBuffer->map();
    // memcpy(pointer, &m_ubo, descriptor.size);
    // m_uniformBuffer->unmap();
}

void TriangleSample::createBindingGroupLayout()
{
    BufferBindingLayout bufferLayout{};
    bufferLayout.index = 0;
    bufferLayout.stages = BindingStageFlagBits::kVertexStage;
    bufferLayout.type = BufferBindingType::kUniform;

    BindingGroupLayoutDescriptor descriptor{};
    descriptor.buffers = { bufferLayout };

    m_bindingGroupLayout = m_device->createBindingGroupLayout(descriptor);
}

void TriangleSample::createBindingGroup()
{
    BufferBinding bufferBinding{
        .index = 0,
        .offset = 0,
        .size = m_uniformBuffer->getSize(),
        .buffer = *m_uniformBuffer,
    };

    BindingGroupDescriptor descriptor{
        .layout = *m_bindingGroupLayout,
        .buffers = { bufferBinding },
    };

    m_bindingGroup = m_device->createBindingGroup(descriptor);
}

void TriangleSample::createRenderPipeline()
{
    // render pipeline layout
    {
        PipelineLayoutDescriptor descriptor{};
        descriptor.layouts = { *m_bindingGroupLayout };

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
        descriptor.code = vertexShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(vertexShaderSource.size());

        vertexShaderModule = m_device->createShaderModule(descriptor);
    }

    // vertex stage

    VertexAttribute positionAttribute{};
    positionAttribute.format = VertexFormat::kSFLOATx3;
    positionAttribute.offset = offsetof(Vertex, pos);
    positionAttribute.location = 0;

    VertexAttribute colorAttribute{};
    colorAttribute.format = VertexFormat::kSFLOATx3;
    colorAttribute.offset = offsetof(Vertex, color);
    colorAttribute.location = 1;

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(Vertex);
    vertexInputLayout.attributes = { positionAttribute, colorAttribute };

    VertexStage vertexStage{
        { *vertexShaderModule, "main" },
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
        { *fragmentShaderModule, "main" },
        { target }
    };

    // depth/stencil

    // render pipeline
    RenderPipelineDescriptor descriptor{
        { *m_renderPipelineLayout },
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
        { 1000, 2000, "Triangle", app },
        ""
    };

    jipu::TriangleSample sample(descriptor);

    sample.exec();
}

#else

int main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::trace);

    jipu::SampleDescriptor descriptor{
        { 800, 600, "Triangle", nullptr },
        argv[0]
    };

    jipu::TriangleSample sample(descriptor);

    return sample.exec();
}

#endif
