#include <math.h>
#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <stdexcept>

#include "camera.h"
#include "file.h"
#include "jipu/native/include/buffer.h"
#include "jipu/native/include/command_buffer.h"
#include "jipu/native/include/command_encoder.h"
#include "jipu/native/include/device.h"
#include "jipu/native/include/instance.h"
#include "jipu/native/include/physical_device.h"
#include "jipu/native/include/pipeline.h"
#include "jipu/native/include/pipeline_layout.h"
#include "jipu/native/include/queue.h"
#include "jipu/native/include/render_pass_encoder.h"
#include "jipu/native/include/surface.h"
#include "jipu/native/include/swapchain.h"
#include "khronos_texture.h"
#include "light.h"
#include "model.h"
#include "sample.h"

namespace jipu
{

class DeferredSample : public Sample
{
public:
    DeferredSample() = delete;
    DeferredSample(const SampleDescriptor& descriptor);
    ~DeferredSample() override;

public:
    void init() override;
    void update() override;
    void draw() override;

private:
    void updateImGui();

private:
    void createDepthStencilTexture();
    void createDepthStencilTextureView();

    void createOffscreenPositionColorAttachmentTexture();
    void createOffscreenPositionColorAttachmentTextureView();
    void createOffscreenNormalColorAttachmentTexture();
    void createOffscreenNormalColorAttachmentTextureView();
    void createOffscreenAlbedoColorAttachmentTexture();
    void createOffscreenAlbedoColorAttachmentTextureView();
    void createOffscreenColorMapTexture();
    void createOffscreenColorMapTextureView();
    void createOffscreenNormalMapTexture();
    void createOffscreenNormalMapTextureView();
    void createOffscreenCamera();
    void createOffscreenUniformBuffer();
    void createOffscreenVertexBuffer();
    void createOffscreenBindingGroupLayout();
    void createOffscreenBindingGroup();
    void createOffscreenPipelineLayout();
    void createOffscreenPipeline();

    void updateOffscreenUniformBuffer();

    void createCompositionBindingGroupLayout();
    void createCompositionBindingGroup();
    void createCompositionPipelineLayout();
    void createCompositionPipeline();
    void createCompositionUniformBuffer();
    void createCompositionVertexBuffer();

    void updateCompositionUniformBuffer();

private:
    struct CompositionUBO
    {
        struct Light
        {
            alignas(16) glm::vec3 position;
            alignas(16) glm::vec3 color;
        };

        std::vector<CompositionUBO::Light> lights{};
        alignas(16) glm::vec3 cameraPosition;
        int lightCount = 8;
        int showTexture = 0;
        int padding1;
        int padding2;
    };

    struct MVP
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } m_mvp;

    struct
    {
        std::unique_ptr<Texture> positionColorAttachmentTexture = nullptr;
        std::unique_ptr<TextureView> positionColorAttachmentTextureView = nullptr;
        std::unique_ptr<Texture> normalColorAttachmentTexture = nullptr;
        std::unique_ptr<TextureView> normalColorAttachmentTextureView = nullptr;
        std::unique_ptr<Texture> albedoColorAttachmentTexture = nullptr;
        std::unique_ptr<TextureView> albedoColorAttachmentTextureView = nullptr;
        std::vector<std::unique_ptr<BindingGroupLayout>> bindingGroupLayouts{};
        std::vector<std::unique_ptr<BindingGroup>> bindingGroups{};
        std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
        std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
        std::unique_ptr<PipelineLayout> pipelineLayout = nullptr;
        std::unique_ptr<RenderPipeline> renderPipeline = nullptr;

        std::unique_ptr<Texture> colorMapTexture = nullptr;
        std::unique_ptr<TextureView> colorMapTextureView = nullptr;
        std::unique_ptr<Texture> normalMapTexture = nullptr;
        std::unique_ptr<TextureView> normalMapTextureView = nullptr;
        std::unique_ptr<Sampler> colorMapSampler = nullptr;
        std::unique_ptr<Sampler> normalMapSampler = nullptr;
        std::unique_ptr<Buffer> uniformBuffer = nullptr;
        std::unique_ptr<Buffer> vertexBuffer = nullptr;
        std::unique_ptr<Buffer> indexBuffer = nullptr;
        std::unique_ptr<Camera> camera = nullptr;
        Polygon polygon{};
    } m_offscreen;

    struct CompositionVertex
    {
        glm::vec3 position;
        glm::vec2 textureCoordinate;
    };
    struct
    {
        std::unique_ptr<Sampler> positionSampler = nullptr;
        std::unique_ptr<Sampler> normalSampler = nullptr;
        std::unique_ptr<Sampler> albedoSampler = nullptr;
        std::vector<std::unique_ptr<BindingGroupLayout>> bindingGroupLayouts{};
        std::vector<std::unique_ptr<BindingGroup>> bindingGroups{};
        std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
        std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
        std::unique_ptr<PipelineLayout> pipelineLayout = nullptr;
        std::unique_ptr<RenderPipeline> renderPipeline = nullptr;

        std::unique_ptr<Buffer> uniformBuffer = nullptr;
        std::unique_ptr<Buffer> vertexBuffer = nullptr;
        CompositionUBO ubo{};
        std::vector<CompositionVertex> vertices{
            { { -1.0, -1.0, 0.0 }, { 0.0, 0.0 } },
            { { -1.0, 1.0, 0.0 }, { 0.0, 1.0 } },
            { { 1.0, -1.0, 0.0 }, { 1.0, 0.0 } },
            { { 1.0, -1.0, 0.0 }, { 1.0, 0.0 } },
            { { -1.0, 1.0, 0.0 }, { 0.0, 1.0 } },
            { { 1.0, 1.0, 0.0 }, { 1.0, 1.0 } },
        };
    } m_composition;

    std::unique_ptr<Texture> m_depthStencilTexture = nullptr;
    std::unique_ptr<TextureView> m_depthStencilTextureView = nullptr;

    uint32_t m_sampleCount = 1; // use only 1, because there is not resolve texture.
    int m_lightMax = 1000;
};

DeferredSample::DeferredSample(const SampleDescriptor& descriptor)
    : Sample(descriptor)
{
    // Do not call init function.
}

DeferredSample::~DeferredSample()
{
    m_depthStencilTextureView.reset();
    m_depthStencilTexture.reset();

    m_composition.vertexBuffer.reset();
    m_composition.uniformBuffer.reset();

    m_composition.vertexShaderModule.reset();
    m_composition.fragmentShaderModule.reset();
    m_composition.renderPipeline.reset();
    m_composition.pipelineLayout.reset();
    m_composition.bindingGroupLayouts.clear();
    m_composition.bindingGroups.clear();
    m_composition.albedoSampler.reset();
    m_composition.normalSampler.reset();
    m_composition.positionSampler.reset();

    m_offscreen.camera.reset();
    m_offscreen.vertexBuffer.reset();
    m_offscreen.indexBuffer.reset();
    m_offscreen.uniformBuffer.reset();
    m_offscreen.normalMapSampler.reset();
    m_offscreen.colorMapSampler.reset();
    m_offscreen.colorMapTextureView.reset();
    m_offscreen.colorMapTexture.reset();
    m_offscreen.normalMapTextureView.reset();
    m_offscreen.normalMapTexture.reset();

    m_offscreen.vertexShaderModule.reset();
    m_offscreen.fragmentShaderModule.reset();
    m_offscreen.renderPipeline.reset();
    m_offscreen.pipelineLayout.reset();
    m_offscreen.bindingGroupLayouts.clear();
    m_offscreen.bindingGroups.clear();
    m_offscreen.albedoColorAttachmentTextureView.reset();
    m_offscreen.albedoColorAttachmentTexture.reset();
    m_offscreen.normalColorAttachmentTextureView.reset();
    m_offscreen.normalColorAttachmentTexture.reset();
    m_offscreen.positionColorAttachmentTextureView.reset();
    m_offscreen.positionColorAttachmentTexture.reset();
}

void DeferredSample::init()
{
    Sample::init();

    createHPCWatcher();

    createDepthStencilTexture();
    createDepthStencilTextureView();

    createOffscreenColorMapTexture();
    createOffscreenColorMapTextureView();
    createOffscreenNormalMapTexture();
    createOffscreenNormalMapTextureView();
    createOffscreenCamera();
    createOffscreenUniformBuffer();
    createOffscreenVertexBuffer();

    createOffscreenPositionColorAttachmentTexture();
    createOffscreenPositionColorAttachmentTextureView();
    createOffscreenNormalColorAttachmentTexture();
    createOffscreenNormalColorAttachmentTextureView();
    createOffscreenAlbedoColorAttachmentTexture();
    createOffscreenAlbedoColorAttachmentTextureView();
    createOffscreenBindingGroupLayout();
    createOffscreenBindingGroup();
    createOffscreenPipelineLayout();
    createOffscreenPipeline();

    createCompositionUniformBuffer();
    createCompositionVertexBuffer();

    createCompositionBindingGroupLayout();
    createCompositionBindingGroup();
    createCompositionPipelineLayout();
    createCompositionPipeline();
}

void DeferredSample::update()
{
    Sample::update();

    updateOffscreenUniformBuffer();
    updateCompositionUniformBuffer();

    updateImGui();
}

void DeferredSample::updateOffscreenUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count() / 5.0f;

    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 80.0f));
    glm::mat4 R1 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 R2 = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 reori = R2 * R1 * T;
    m_mvp.model = reori;

    void* mappedPointer = m_offscreen.uniformBuffer->map();
    memcpy(mappedPointer, &m_mvp, sizeof(MVP));
}

void DeferredSample::updateCompositionUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    float ratio = sin(time);
    for (auto i = 0; i < m_lightMax; ++i)
    {
        auto& light = m_composition.ubo.lights[i];
        light.position[0] = light.position[0] * ratio;
        light.position[1] = light.position[1] * ratio;
        // light.position[2] = light.position[2] * ratio;
    }
    m_composition.ubo.cameraPosition = m_offscreen.camera->getPosition();

    uint32_t lightSize = static_cast<uint32_t>(sizeof(CompositionUBO::Light) * m_lightMax);
    uint32_t lightCountByteSize = static_cast<uint32_t>(sizeof(int));
    uint32_t showTextureByteSize = static_cast<uint32_t>(sizeof(int));
    uint32_t cameraPositionSize = sizeof(glm::vec3);

    auto& uniformBuffer = m_composition.uniformBuffer;
    void* pointer = uniformBuffer->map();
    char* bytePointer = static_cast<char*>(pointer);

    memcpy(bytePointer, m_composition.ubo.lights.data(), lightSize);
    bytePointer += lightSize;

    memcpy(bytePointer, &m_composition.ubo.cameraPosition, cameraPositionSize);
    bytePointer += cameraPositionSize;

    memcpy(bytePointer, &m_composition.ubo.lightCount, lightCountByteSize);
    bytePointer += lightCountByteSize;

    memcpy(bytePointer, &m_composition.ubo.showTexture, showTextureByteSize);
}

void DeferredSample::updateImGui()
{
    recordImGui({ [&]() {
        windowImGui("Settings",
                    { [&]() {
                         ImGui::SliderInt("Number of Light", &m_composition.ubo.lightCount, 1, m_lightMax);
                     },
                      [&]() {
                          if (ImGui::RadioButton("Deferred", m_composition.ubo.showTexture == 0))
                              m_composition.ubo.showTexture = 0;
                          else if (ImGui::RadioButton("Position", m_composition.ubo.showTexture == 1))
                              m_composition.ubo.showTexture = 1;
                          else if (ImGui::RadioButton("Normal", m_composition.ubo.showTexture == 2))
                              m_composition.ubo.showTexture = 2;
                          else if (ImGui::RadioButton("Albedo", m_composition.ubo.showTexture == 3))
                              m_composition.ubo.showTexture = 3;
                      } });
        profilingWindow();
    } });
}

void DeferredSample::draw()
{
    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);

    auto renderView = m_swapchain->acquireNextTextureView();

    {
        ColorAttachment positionColorAttachment{
            .renderView = m_offscreen.positionColorAttachmentTextureView.get()
        };
        positionColorAttachment.loadOp = LoadOp::kClear;
        positionColorAttachment.storeOp = StoreOp::kStore;
        positionColorAttachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };

        ColorAttachment normalColorAttachment{
            .renderView = m_offscreen.normalColorAttachmentTextureView.get()
        };
        normalColorAttachment.loadOp = LoadOp::kClear;
        normalColorAttachment.storeOp = StoreOp::kStore;
        normalColorAttachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };

        ColorAttachment albedoColorAttachment{
            .renderView = m_offscreen.albedoColorAttachmentTextureView.get()
        };
        albedoColorAttachment.loadOp = LoadOp::kClear;
        albedoColorAttachment.storeOp = StoreOp::kStore;
        albedoColorAttachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };

        DepthStencilAttachment depthStencilAttachment{
            .textureView = m_depthStencilTextureView.get()
        };
        depthStencilAttachment.depthLoadOp = LoadOp::kClear;
        depthStencilAttachment.depthStoreOp = StoreOp::kStore;
        depthStencilAttachment.clearValue = { .depth = 1.0f, .stencil = 0 };

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { positionColorAttachment, normalColorAttachment, albedoColorAttachment },
            .depthStencilAttachment = depthStencilAttachment
        };

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_offscreen.renderPipeline.get());
        renderPassEncoder->setVertexBuffer(0, *m_offscreen.vertexBuffer);
        renderPassEncoder->setIndexBuffer(*m_offscreen.indexBuffer, IndexFormat::kUint16);
        renderPassEncoder->setBindingGroup(0, *m_offscreen.bindingGroups[0]);
        renderPassEncoder->setBindingGroup(1, *m_offscreen.bindingGroups[1]);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_offscreen.polygon.indices.size()), 1, 0, 0, 0);
        renderPassEncoder->end();
    }

    {
        ColorAttachment colorAttachment{
            .renderView = renderView
        };
        colorAttachment.loadOp = LoadOp::kClear;
        colorAttachment.storeOp = StoreOp::kStore;
        colorAttachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };

        DepthStencilAttachment depthStencilAttachment{
            .textureView = m_depthStencilTextureView.get()
        };
        depthStencilAttachment.depthLoadOp = LoadOp::kClear;
        depthStencilAttachment.depthStoreOp = StoreOp::kStore;
        depthStencilAttachment.clearValue = { .depth = 1.0f, .stencil = 0 };

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { colorAttachment },
            .depthStencilAttachment = depthStencilAttachment
        };

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_composition.renderPipeline.get());
        renderPassEncoder->setVertexBuffer(0, *m_composition.vertexBuffer);
        renderPassEncoder->setBindingGroup(0, *m_composition.bindingGroups[0]);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->draw(static_cast<uint32_t>(m_composition.vertices.size()), 1, 0, 0);
        renderPassEncoder->end();
    }

    drawImGui(commandEncoder.get(), *renderView);

    CommandBufferDescriptor descriptor{};
    auto commandBuffer = commandEncoder->finish(descriptor);

    m_queue->submit({ commandBuffer.get() });
    m_swapchain->present();
}

void DeferredSample::createOffscreenPositionColorAttachmentTexture()
{
    TextureDescriptor descriptor{};
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kRGBA16Unorm;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = m_sampleCount;
    descriptor.width = m_swapchain->getWidth();
    descriptor.height = m_swapchain->getHeight();
    descriptor.depth = 1;
    descriptor.usage = TextureUsageFlagBits::kColorAttachment | TextureUsageFlagBits::kTextureBinding;

    m_offscreen.positionColorAttachmentTexture = m_device->createTexture(descriptor);
}

void DeferredSample::createOffscreenPositionColorAttachmentTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_offscreen.positionColorAttachmentTextureView = m_offscreen.positionColorAttachmentTexture->createTextureView(descriptor);
}

void DeferredSample::createOffscreenNormalColorAttachmentTexture()
{
    TextureDescriptor descriptor{};
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kRGBA16Unorm;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = m_sampleCount;
    descriptor.width = m_swapchain->getWidth();
    descriptor.height = m_swapchain->getHeight();
    descriptor.depth = 1;
    descriptor.usage = TextureUsageFlagBits::kColorAttachment |
                       TextureUsageFlagBits::kTextureBinding;

    m_offscreen.normalColorAttachmentTexture = m_device->createTexture(descriptor);
}

void DeferredSample::createOffscreenNormalColorAttachmentTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_offscreen.normalColorAttachmentTextureView = m_offscreen.normalColorAttachmentTexture->createTextureView(descriptor);
}

void DeferredSample::createOffscreenAlbedoColorAttachmentTexture()
{
    TextureDescriptor descriptor{};
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kRGBA8Unorm;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = m_sampleCount;
    descriptor.width = m_swapchain->getWidth();
    descriptor.height = m_swapchain->getHeight();
    descriptor.depth = 1;
    descriptor.usage = TextureUsageFlagBits::kColorAttachment | TextureUsageFlagBits::kTextureBinding;

    m_offscreen.albedoColorAttachmentTexture = m_device->createTexture(descriptor);
}

void DeferredSample::createOffscreenAlbedoColorAttachmentTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_offscreen.albedoColorAttachmentTextureView = m_offscreen.albedoColorAttachmentTexture->createTextureView(descriptor);
}

void DeferredSample::createOffscreenColorMapTexture()
{
    // ktx{ m_appDir / "colormap_rgba.ktx" };
    std::vector<char> data = utils::readFile(m_appDir / "colormap_rgba.ktx", m_handle);
    KTX ktx{ data.data(), data.size() };

    TextureDescriptor textureDescriptor{};
    textureDescriptor.type = TextureType::k2D;
    textureDescriptor.format = TextureFormat::kRGBA8Unorm; // kRGBA8UnormSrgb
    textureDescriptor.mipLevels = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.width = ktx.getWidth();
    textureDescriptor.height = ktx.getHeight();
    textureDescriptor.depth = 1;
    textureDescriptor.usage = TextureUsageFlagBits::kCopySrc |
                              TextureUsageFlagBits::kCopyDst |
                              TextureUsageFlagBits::kTextureBinding,

    m_offscreen.colorMapTexture = m_device->createTexture(textureDescriptor);

    // copy texture data
    {
        BufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = ktx.getWidth() * ktx.getHeight() * ktx.getChannel() * sizeof(char);
        bufferDescriptor.usage = BufferUsageFlagBits::kCopySrc;

        auto stagingBuffer = m_device->createBuffer(bufferDescriptor);
        void* pointer = stagingBuffer->map();
        memcpy(pointer, ktx.getPixels(), bufferDescriptor.size);
        // stagingBuffer->unmap();

        BlitTextureBuffer blitTextureBuffer{
            .buffer = stagingBuffer.get(),
            .offset = 0,
            .bytesPerRow = static_cast<uint32_t>(ktx.getWidth() * ktx.getChannel() * sizeof(char)),
            .rowsPerTexture = static_cast<uint32_t>(ktx.getHeight()),
        };

        BlitTexture blitTexture{
            .texture = m_offscreen.colorMapTexture.get(),
            .aspect = TextureAspectFlagBits::kColor,
        };

        Extent3D extent{};
        extent.width = ktx.getWidth();
        extent.height = ktx.getHeight();
        extent.depth = 1;

        CommandEncoderDescriptor commandEncoderDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);

        commandEncoder->copyBufferToTexture(blitTextureBuffer, blitTexture, extent);
        CommandBufferDescriptor commandBufferDescriptor{};
        auto commandBuffer = commandEncoder->finish(commandBufferDescriptor);

        m_queue->submit({ commandBuffer.get() });
    }
}

void DeferredSample::createOffscreenColorMapTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_offscreen.colorMapTextureView = m_offscreen.colorMapTexture->createTextureView(descriptor);
}

void DeferredSample::createOffscreenNormalMapTexture()
{
    // KTX ktx{ m_appDir / "normalmap_rgba.ktx" };
    std::vector<char> data = utils::readFile(m_appDir / "normalmap_rgba.ktx", m_handle);
    KTX ktx{ data.data(), data.size() };

    TextureDescriptor textureDescriptor{};
    textureDescriptor.type = TextureType::k2D;
    textureDescriptor.format = TextureFormat::kRGBA8Unorm;
    textureDescriptor.mipLevels = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.width = ktx.getWidth();
    textureDescriptor.height = ktx.getHeight();
    textureDescriptor.depth = 1;
    textureDescriptor.usage = TextureUsageFlagBits::kCopySrc |
                              TextureUsageFlagBits::kCopyDst |
                              TextureUsageFlagBits::kTextureBinding,

    m_offscreen.normalMapTexture = m_device->createTexture(textureDescriptor);

    // copy texture data
    {
        BufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = ktx.getWidth() * ktx.getHeight() * ktx.getChannel() * sizeof(char);
        bufferDescriptor.usage = BufferUsageFlagBits::kCopySrc;

        auto stagingBuffer = m_device->createBuffer(bufferDescriptor);
        void* pointer = stagingBuffer->map();
        memcpy(pointer, ktx.getPixels(), bufferDescriptor.size);
        // stagingBuffer->unmap();

        BlitTextureBuffer blitTextureBuffer{
            .buffer = stagingBuffer.get(),
            .offset = 0,
            .bytesPerRow = static_cast<uint32_t>(ktx.getWidth() * ktx.getChannel() * sizeof(char)),
            .rowsPerTexture = static_cast<uint32_t>(ktx.getHeight())
        };

        BlitTexture blitTexture{
            .texture = m_offscreen.normalMapTexture.get(),
            .aspect = TextureAspectFlagBits::kColor,
        };

        Extent3D extent{};
        extent.width = ktx.getWidth();
        extent.height = ktx.getHeight();
        extent.depth = 1;

        CommandEncoderDescriptor commandEncoderDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);

        commandEncoder->copyBufferToTexture(blitTextureBuffer, blitTexture, extent);

        CommandBufferDescriptor commandBufferDescriptor{};
        auto commandBuffer = commandEncoder->finish(commandBufferDescriptor);

        m_queue->submit({ commandBuffer.get() });
    }
}

void DeferredSample::createOffscreenNormalMapTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_offscreen.normalMapTextureView = m_offscreen.normalMapTexture->createTextureView(descriptor);
}

void DeferredSample::createOffscreenCamera()
{
    m_offscreen.camera = std::make_unique<PerspectiveCamera>(glm::radians(45.0f),
                                                             m_swapchain->getWidth() / static_cast<float>(m_swapchain->getHeight()),
                                                             0.1f,
                                                             1000.0f);
    m_offscreen.camera->lookAt(glm::vec3(0.0f, 0.0f, 300.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
}

void DeferredSample::createOffscreenUniformBuffer()
{
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 80.0f));
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 reori = R * T;
    m_mvp.model = reori;
    m_mvp.view = m_offscreen.camera->getViewMat();
    m_mvp.proj = m_offscreen.camera->getProjectionMat();

    BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = sizeof(MVP);
    bufferDescriptor.usage = BufferUsageFlagBits::kUniform;

    m_offscreen.uniformBuffer = m_device->createBuffer(bufferDescriptor);
    void* mappedPointer = m_offscreen.uniformBuffer->map();
    memcpy(mappedPointer, &m_mvp, sizeof(MVP));
    // m_offscreen.uniformBuffer->unmap();
}

void DeferredSample::createOffscreenVertexBuffer()
{
    // m_offscreen.polygon = loadGLTF(m_appDir / "armor.gltf");
    std::vector<char> data = utils::readFile(m_appDir / "armor.gltf", m_handle);
    m_offscreen.polygon = loadGLTF(data.data(), data.size(), m_appDir);

    {
        BufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = sizeof(Vertex) * m_offscreen.polygon.vertices.size();
        bufferDescriptor.usage = BufferUsageFlagBits::kVertex;

        m_offscreen.vertexBuffer = m_device->createBuffer(bufferDescriptor);
        void* mappedPointer = m_offscreen.vertexBuffer->map();
        memcpy(mappedPointer, m_offscreen.polygon.vertices.data(), bufferDescriptor.size);
        m_offscreen.vertexBuffer->unmap();
    }
    {
        BufferDescriptor bufferDescriptor{};
        bufferDescriptor.size = sizeof(uint16_t) * m_offscreen.polygon.indices.size();
        bufferDescriptor.usage = BufferUsageFlagBits::kIndex;

        m_offscreen.indexBuffer = m_device->createBuffer(bufferDescriptor);
        void* mappedPointer = m_offscreen.indexBuffer->map();
        memcpy(mappedPointer, m_offscreen.polygon.indices.data(), bufferDescriptor.size);
        m_offscreen.indexBuffer->unmap();
    }
}

void DeferredSample::createOffscreenBindingGroupLayout()
{
    m_offscreen.bindingGroupLayouts.resize(2);

    {
        BufferBindingLayout bufferBindingLayout{};
        bufferBindingLayout.type = BufferBindingType::kUniform;
        bufferBindingLayout.index = 0;
        bufferBindingLayout.stages = BindingStageFlagBits::kVertexStage;

        BindingGroupLayoutDescriptor bindingGroupLayoutDescriptor{};
        bindingGroupLayoutDescriptor.buffers = { bufferBindingLayout };

        m_offscreen.bindingGroupLayouts[0] = m_device->createBindingGroupLayout(bindingGroupLayoutDescriptor);
    }

    {
        SamplerBindingLayout colorSamplerBindingLayout{};
        colorSamplerBindingLayout.index = 0;
        colorSamplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

        SamplerBindingLayout normalSamplerBindingLayout{};
        normalSamplerBindingLayout.index = 1;
        normalSamplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

        TextureBindingLayout colorTextureBindingLayout{};
        colorTextureBindingLayout.index = 2;
        colorTextureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

        TextureBindingLayout normalTextureBindingLayout{};
        normalTextureBindingLayout.index = 3;
        normalTextureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

        BindingGroupLayoutDescriptor bindingGroupLayoutDescriptor{};
        bindingGroupLayoutDescriptor.samplers = { colorSamplerBindingLayout, normalSamplerBindingLayout };
        bindingGroupLayoutDescriptor.textures = { colorTextureBindingLayout, normalTextureBindingLayout };

        m_offscreen.bindingGroupLayouts[1] = m_device->createBindingGroupLayout(bindingGroupLayoutDescriptor);
    }
}

void DeferredSample::createOffscreenBindingGroup()
{
    { // create color map sampler

        SamplerDescriptor samplerDescriptor{};
        samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
        samplerDescriptor.lodMin = 0.0f;
        samplerDescriptor.lodMax = static_cast<float>(m_offscreen.colorMapTexture->getMipLevels());
        samplerDescriptor.minFilter = FilterMode::kLinear;
        samplerDescriptor.magFilter = FilterMode::kLinear;
        samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;

        m_offscreen.colorMapSampler = m_device->createSampler(samplerDescriptor);
    }

    { // create normal map sampler

        SamplerDescriptor samplerDescriptor{};
        samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
        samplerDescriptor.lodMin = 0.0f;
        samplerDescriptor.lodMax = static_cast<float>(m_offscreen.normalMapTexture->getMipLevels());
        samplerDescriptor.minFilter = FilterMode::kLinear;
        samplerDescriptor.magFilter = FilterMode::kLinear;
        samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;

        m_offscreen.normalMapSampler = m_device->createSampler(samplerDescriptor);
    }

    m_offscreen.bindingGroups.resize(2);
    {
        BufferBinding bufferBinding{
            .index = 0,
            .offset = 0,
            .size = sizeof(MVP),
            .buffer = m_offscreen.uniformBuffer.get(),
        };

        BindingGroupDescriptor bindingGroupDescriptor{
            .layout = m_offscreen.bindingGroupLayouts[0].get(),
            .buffers = { bufferBinding },
        };

        m_offscreen.bindingGroups[0] = m_device->createBindingGroup(bindingGroupDescriptor);
    }

    {
        SamplerBinding colorSamplerBinding{
            .index = 0,
            .sampler = m_offscreen.colorMapSampler.get(),
        };

        SamplerBinding normalSamplerBinding{
            .index = 1,
            .sampler = m_offscreen.normalMapSampler.get(),
        };

        TextureBinding colorTextureBinding{
            .index = 2,
            .textureView = m_offscreen.colorMapTextureView.get(),
        };

        TextureBinding normalTextureBinding{
            .index = 3,
            .textureView = m_offscreen.normalMapTextureView.get(),
        };

        BindingGroupDescriptor bindingGroupDescriptor{
            .layout = m_offscreen.bindingGroupLayouts[1].get(),
            .samplers = { colorSamplerBinding, normalSamplerBinding },
            .textures = { colorTextureBinding, normalTextureBinding },
        };

        m_offscreen.bindingGroups[1] = m_device->createBindingGroup(bindingGroupDescriptor);
    }
}

void DeferredSample::createOffscreenPipelineLayout()
{
    PipelineLayoutDescriptor descriptor{};
    descriptor.layouts = { m_offscreen.bindingGroupLayouts[0].get(), m_offscreen.bindingGroupLayouts[1].get() };

    m_offscreen.pipelineLayout = m_device->createPipelineLayout(descriptor);
}

void DeferredSample::createOffscreenPipeline()
{
    // Input Assembly
    InputAssemblyStage inputAssembly{};
    inputAssembly.topology = PrimitiveTopology::kTriangleList;

    // shader module
    {
        std::vector<char> vertexSource = utils::readFile(m_appDir / "offscreen.vert.spv", m_handle);

        ShaderModuleDescriptor shaderModuleDescriptor;
        shaderModuleDescriptor.code = vertexSource.data();
        shaderModuleDescriptor.codeSize = vertexSource.size();

        m_offscreen.vertexShaderModule = m_device->createShaderModule(shaderModuleDescriptor);
    }

    // Vertex Shader

    // input layout
    VertexInputLayout inputLayout;
    inputLayout.mode = VertexMode::kVertex;
    inputLayout.stride = sizeof(Vertex);

    VertexAttribute positionAttribute;
    positionAttribute.format = VertexFormat::kFloat32x3;
    positionAttribute.offset = offsetof(Vertex, pos);
    positionAttribute.location = 0;

    VertexAttribute normalAttribute;
    normalAttribute.format = VertexFormat::kFloat32x3;
    normalAttribute.offset = offsetof(Vertex, normal);
    normalAttribute.location = 1;

    VertexAttribute tangentAttribute;
    tangentAttribute.format = VertexFormat::kFloat32x4;
    tangentAttribute.offset = offsetof(Vertex, tangent);
    tangentAttribute.location = 2;

    VertexAttribute texCoordAttribute;
    texCoordAttribute.format = VertexFormat::kFloat32x2;
    texCoordAttribute.offset = offsetof(Vertex, texCoord);
    texCoordAttribute.location = 3;

    inputLayout.attributes = { positionAttribute, normalAttribute, tangentAttribute, texCoordAttribute };

    VertexStage vertexShage{
        { m_offscreen.vertexShaderModule.get(), "main" },
        { inputLayout }
    };

    // Rasterization
    RasterizationStage rasterizationStage{};
    rasterizationStage.sampleCount = m_sampleCount;
    rasterizationStage.cullMode = CullMode::kNone;
    rasterizationStage.frontFace = FrontFace::kCounterClockwise;

    // shader module
    {
        std::vector<char> fragmentSource = utils::readFile(m_appDir / "offscreen.frag.spv", m_handle);

        ShaderModuleDescriptor shaderModuleDescriptor;
        shaderModuleDescriptor.code = fragmentSource.data();
        shaderModuleDescriptor.codeSize = fragmentSource.size();

        m_offscreen.fragmentShaderModule = m_device->createShaderModule(shaderModuleDescriptor);
    }

    // Fragment Shader

    // targets
    FragmentStage::Target positionTarget{};
    positionTarget.format = m_offscreen.positionColorAttachmentTexture->getFormat();

    FragmentStage::Target normalTarget{};
    normalTarget.format = m_offscreen.normalColorAttachmentTexture->getFormat();

    FragmentStage::Target albedoTarget{};
    albedoTarget.format = m_offscreen.albedoColorAttachmentTexture->getFormat();

    FragmentStage fragmentStage{
        { m_offscreen.fragmentShaderModule.get(), "main" },
        { positionTarget, normalTarget, albedoTarget }
    };

    DepthStencilStage depthStencil{};
    depthStencil.format = m_depthStencilTexture->getFormat();

    RenderPipelineDescriptor descriptor{
        m_offscreen.pipelineLayout.get(),
        inputAssembly,
        vertexShage,
        rasterizationStage,
        fragmentStage,
        depthStencil
    };

    m_offscreen.renderPipeline = m_device->createRenderPipeline(descriptor);
}

void DeferredSample::createCompositionBindingGroupLayout()
{
    m_composition.bindingGroupLayouts.resize(1);

    BindingGroupLayoutDescriptor descriptor{};

    SamplerBindingLayout positionSamplerBindingLayout{};
    positionSamplerBindingLayout.index = 0;
    positionSamplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    SamplerBindingLayout normalSamplerBindingLayout{};
    normalSamplerBindingLayout.index = 1;
    normalSamplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    SamplerBindingLayout albedoSamplerBindingLayout{};
    albedoSamplerBindingLayout.index = 2;
    albedoSamplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    TextureBindingLayout positionTextureBindingLayout{};
    positionTextureBindingLayout.index = 3;
    positionTextureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    TextureBindingLayout normalTextureBindingLayout{};
    normalTextureBindingLayout.index = 4;
    normalTextureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    TextureBindingLayout albedoTextureBindingLayout{};
    albedoTextureBindingLayout.index = 5;
    albedoTextureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

    BufferBindingLayout uniformBufferBindingLayout{};
    uniformBufferBindingLayout.index = 6;
    uniformBufferBindingLayout.stages = BindingStageFlagBits::kFragmentStage;
    uniformBufferBindingLayout.type = BufferBindingType::kUniform;

    descriptor.buffers = { uniformBufferBindingLayout };
    descriptor.samplers = { positionSamplerBindingLayout, normalSamplerBindingLayout, albedoSamplerBindingLayout };
    descriptor.textures = { positionTextureBindingLayout, normalTextureBindingLayout, albedoTextureBindingLayout };

    m_composition.bindingGroupLayouts[0] = m_device->createBindingGroupLayout(descriptor);
}

void DeferredSample::createCompositionBindingGroup()
{
    m_composition.bindingGroups.resize(1);

    {
        SamplerDescriptor samplerDescriptor{};
        samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
        samplerDescriptor.magFilter = FilterMode::kLinear;
        samplerDescriptor.minFilter = FilterMode::kLinear;
        samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;
        samplerDescriptor.lodMin = 0.0f;
        samplerDescriptor.lodMax = static_cast<float>(m_offscreen.positionColorAttachmentTexture->getMipLevels());

        m_composition.positionSampler = m_device->createSampler(samplerDescriptor);
    }

    SamplerBinding positionSamplerBinding{
        .index = 0,
        .sampler = m_composition.positionSampler.get()
    };

    {
        SamplerDescriptor samplerDescriptor{};
        samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
        samplerDescriptor.magFilter = FilterMode::kLinear;
        samplerDescriptor.minFilter = FilterMode::kLinear;
        samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;
        samplerDescriptor.lodMin = 0.0f;
        samplerDescriptor.lodMax = static_cast<float>(m_offscreen.normalColorAttachmentTexture->getMipLevels());

        m_composition.normalSampler = m_device->createSampler(samplerDescriptor);
    }

    SamplerBinding normalSamplerBinding{
        .index = 1,
        .sampler = m_composition.normalSampler.get()

    };

    {
        SamplerDescriptor samplerDescriptor{};
        samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
        samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
        samplerDescriptor.magFilter = FilterMode::kLinear;
        samplerDescriptor.minFilter = FilterMode::kLinear;
        samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;
        samplerDescriptor.lodMin = 0.0f;
        samplerDescriptor.lodMax = static_cast<float>(m_offscreen.albedoColorAttachmentTexture->getMipLevels());

        m_composition.albedoSampler = m_device->createSampler(samplerDescriptor);
    }

    SamplerBinding albedoSamplerBinding{
        .index = 2,
        .sampler = m_composition.albedoSampler.get()

    };

    // positionTextureBinding.index = 3;
    // positionTextureBinding.textureView = m_offscreen.positionColorAttachmentTextureView.get();
    TextureBinding positionTextureBinding{
        .index = 3,
        .textureView = m_offscreen.positionColorAttachmentTextureView.get()
    };

    TextureBinding normalTextureBinding{
        .index = 4,
        .textureView = m_offscreen.normalColorAttachmentTextureView.get()
    };

    TextureBinding albedoTextureBinding{
        .index = 5,
        .textureView = m_offscreen.albedoColorAttachmentTextureView.get()
    };

    BufferBinding uniformBufferBinding{
        .index = 6,
        .offset = 0,
        .size = m_composition.uniformBuffer->getSize(),
        .buffer = m_composition.uniformBuffer.get(),
    };

    BindingGroupDescriptor descriptor{
        .layout = m_composition.bindingGroupLayouts[0].get(),
        .buffers = { uniformBufferBinding },
        .samplers = { positionSamplerBinding, normalSamplerBinding, albedoSamplerBinding },
        .textures = { positionTextureBinding, normalTextureBinding, albedoTextureBinding }
    };

    m_composition.bindingGroups[0] = m_device->createBindingGroup(descriptor);
}

void DeferredSample::createCompositionPipelineLayout()
{
    PipelineLayoutDescriptor descriptor{};
    descriptor.layouts = { m_composition.bindingGroupLayouts[0].get() };

    m_composition.pipelineLayout = m_device->createPipelineLayout(descriptor);
}

void DeferredSample::createCompositionPipeline()
{
    // Input Assembly
    InputAssemblyStage inputAssemblyStage{};
    inputAssemblyStage.topology = PrimitiveTopology::kTriangleList;

    // Vertex
    std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;

    // vertex layout
    VertexInputLayout vertexInputLayout{};
    { // vertex attribute
        std::vector<VertexAttribute> attributes(2);

        VertexAttribute positionAttribute{};
        positionAttribute.format = VertexFormat::kFloat32x3;
        positionAttribute.offset = offsetof(CompositionVertex, position);
        positionAttribute.location = 0;
        attributes[0] = positionAttribute;

        VertexAttribute texCoordAttribute{};
        texCoordAttribute.format = VertexFormat::kFloat32x2;
        texCoordAttribute.offset = offsetof(CompositionVertex, textureCoordinate);
        texCoordAttribute.location = 1;
        attributes[1] = texCoordAttribute;

        vertexInputLayout.attributes = attributes;
    }
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(CompositionVertex);

    // vertex shader module
    std::vector<char> vertexSource = utils::readFile(m_appDir / "composition.vert.spv", m_handle);

    ShaderModuleDescriptor shaderModuleDescriptor{};
    shaderModuleDescriptor.code = vertexSource.data();
    shaderModuleDescriptor.codeSize = static_cast<uint32_t>(vertexSource.size());
    vertexShaderModule = m_device->createShaderModule(shaderModuleDescriptor);

    VertexStage vertexStage{
        { vertexShaderModule.get(), "main" },
        { vertexInputLayout }
    };

    // Rasterization
    RasterizationStage rasterizationStage{};
    rasterizationStage.cullMode = CullMode::kBack;
    rasterizationStage.frontFace = FrontFace::kCounterClockwise;
    rasterizationStage.sampleCount = m_sampleCount;

    // Fragment

    // fragment shader module
    std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
    std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "composition.frag.spv", m_handle);

    ShaderModuleDescriptor fragShaderModuleDescriptor{};
    fragShaderModuleDescriptor.code = fragmentShaderSource.data();
    fragShaderModuleDescriptor.codeSize = fragmentShaderSource.size();
    fragmentShaderModule = m_device->createShaderModule(fragShaderModuleDescriptor);

    FragmentStage::Target target{};
    target.format = m_swapchain->getTextureFormat();

    FragmentStage fragmentStage{
        { fragmentShaderModule.get(), "main" },
        { target }
    };

    // DepthStencil
    DepthStencilStage depthStencilStage{};
    depthStencilStage.format = m_depthStencilTexture->getFormat();

    RenderPipelineDescriptor renderPipelineDescriptor{
        m_composition.pipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage,
        depthStencilStage
    };

    m_composition.renderPipeline = m_device->createRenderPipeline(renderPipelineDescriptor);
}

void DeferredSample::createCompositionUniformBuffer()
{
    // light
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> xDirDist(0, 1);
        std::uniform_int_distribution<> yDirDist(0, 1);
        std::uniform_int_distribution<> zDirDist(0, 1);
        std::uniform_real_distribution<> xDist(30.0f, 30.0f);
        std::uniform_real_distribution<> yDist(30.0f, 30.0f);
        std::uniform_real_distribution<> zDist(30.0f, 30.0f);
        std::uniform_real_distribution<> rDist(0.0f, 1.0f);
        std::uniform_real_distribution<> gDist(0.0f, 1.0f);
        std::uniform_real_distribution<> bDist(0.0f, 1.0f);

        for (int i = 0; i < m_lightMax; i++)
        {
            auto xDir = xDirDist(gen) == 0 ? -1 : 1;
            auto yDir = yDirDist(gen) == 0 ? -1 : 1;
            auto zDir = zDirDist(gen) == 0 ? -1 : 1;
            auto x = xDist(gen);
            auto y = yDist(gen);
            auto z = zDist(gen);
            auto r = rDist(gen);
            auto g = gDist(gen);
            auto b = bDist(gen);

            // spdlog::trace("xDir: {}, yDir: {}, zDir: {}, x: {}, y: {}, z: {}", xDir, yDir, zDir, x, y, z);
            // spdlog::trace("r: {}, g: {}, b: {}", r, g, b);

            CompositionUBO::Light light{ { xDir * x, yDir * y, zDir * z },
                                         { r, g, b } };
            m_composition.ubo.lights.push_back(light);
        }
    }
    // camera position
    {
        m_composition.ubo.cameraPosition = m_offscreen.camera->getPosition();
    }

    uint32_t lightSize = static_cast<uint32_t>(sizeof(CompositionUBO::Light) * m_lightMax);
    uint32_t lightCountByteSize = sizeof(int);
    uint32_t showTextureByteSize = sizeof(int);
    uint32_t cameraPositionSize = sizeof(glm::vec3);

    BufferDescriptor descriptor{};
    descriptor.size = lightSize + cameraPositionSize + lightCountByteSize + showTextureByteSize;
    descriptor.size += sizeof(int) * 2; // padding
    descriptor.usage = BufferUsageFlagBits::kUniform;

    m_composition.uniformBuffer = m_device->createBuffer(descriptor);

    auto& uniformBuffer = m_composition.uniformBuffer;
    void* pointer = uniformBuffer->map();
    char* bytePointer = static_cast<char*>(pointer);
    memcpy(bytePointer, m_composition.ubo.lights.data(), lightSize);
    bytePointer += lightSize;
    memcpy(bytePointer, &m_composition.ubo.cameraPosition, cameraPositionSize);
    bytePointer += cameraPositionSize;
    memcpy(bytePointer, &m_composition.ubo.lightCount, lightCountByteSize);
    bytePointer += lightCountByteSize;
    memcpy(bytePointer, &m_composition.ubo.showTexture, showTextureByteSize);
    // uniformBuffer->unmap();
}

void DeferredSample::createCompositionVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = sizeof(CompositionVertex) * m_composition.vertices.size();
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_composition.vertexBuffer = m_device->createBuffer(descriptor);

    auto& vertexBuffer = m_composition.vertexBuffer;
    void* mappedPointer = vertexBuffer->map();
    memcpy(mappedPointer, m_composition.vertices.data(), descriptor.size);
    vertexBuffer->unmap();
}

void DeferredSample::createDepthStencilTexture()
{
    TextureDescriptor descriptor{};
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kDepth32Float;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = m_sampleCount;
    descriptor.width = m_swapchain->getWidth();
    descriptor.height = m_swapchain->getHeight();
    descriptor.depth = 1;
    descriptor.usage = TextureUsageFlagBits::kDepthStencil;

    m_depthStencilTexture = m_device->createTexture(descriptor);
}

void DeferredSample::createDepthStencilTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kDepth;

    m_depthStencilTextureView = m_depthStencilTexture->createTextureView(descriptor);
}

} // namespace jipu

#if defined(__ANDROID__) || defined(ANDROID)

// GameActivity's C/C++ code
#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

// Glue from GameActivity to android_main()
// Passing GameActivity event from main thread to app native thread.
extern "C"
{
#include <game-activity/native_app_glue/android_native_app_glue.c>
}

void android_main(struct android_app* app)
{
    jipu::SampleDescriptor descriptor{
        { 1000, 2000, "Deferred Sample", app },
        ""
    };

    jipu::DeferredSample sample(descriptor);

    sample.exec();
}

#else

int main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::trace);

    jipu::SampleDescriptor descriptor{
        { 800, 600, "Deferred Sample", nullptr },
        argv[0]
    };

    jipu::DeferredSample sample(descriptor);

    return sample.exec();
}

#endif