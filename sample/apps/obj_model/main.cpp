

#include "file.h"
#include "image.h"
#include "model.h"
#include "native_sample.h"

#include "jipu/native/adapter.h"
#include "jipu/native/bind_group.h"
#include "jipu/native/bind_group_layout.h"
#include "jipu/native/buffer.h"
#include "jipu/native/command_buffer.h"
#include "jipu/native/device.h"
#include "jipu/native/physical_device.h"
#include "jipu/native/pipeline.h"
#include "jipu/native/pipeline_layout.h"
#include "jipu/native/queue.h"
#include "jipu/native/sampler.h"
#include "jipu/native/shader_module.h"
#include "jipu/native/surface.h"
#include "jipu/native/swapchain.h"
#include "jipu/native/texture_view.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <spdlog/spdlog.h>
#include <stddef.h>

namespace jipu
{

class OBJModelSample : public NativeSample
{
public:
    OBJModelSample() = delete;
    OBJModelSample(const SampleDescriptor& descriptor);
    ~OBJModelSample() override;

    void init() override;
    void onUpdate() override;
    void onDraw() override;

private:
    void updateImGui();

private:
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();

    void createImageTexture();
    void createImageTextureView();
    void createImageSampler();

    void createColorAttachmentTexture();
    void createColorAttachmentTextureView();
    void createDepthStencilTexture();
    void createDepthStencilTextureView();

    void createBindGroupLayout();
    void createBindGroup();

    void createPipelineLayout();
    void createRenderPipeline();

    void copyBufferToBuffer(Buffer& src, Buffer& dst);
    void copyBufferToTexture(Buffer& imageTextureBuffer, Texture& imageTexture);

    void updateUniformBuffer();

private:
    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    // data
    Polygon m_polygon{};
    std::unique_ptr<Image> m_image = nullptr;

    std::unique_ptr<Buffer> m_vertexBuffer = nullptr;
    std::unique_ptr<Buffer> m_indexBuffer = nullptr;

    std::unique_ptr<Texture> m_imageTexture = nullptr;
    std::unique_ptr<TextureView> m_imageTextureView = nullptr;
    std::unique_ptr<Sampler> m_imageSampler = nullptr;

    std::unique_ptr<Buffer> m_uniformBuffer = nullptr;
    void* m_uniformBufferMappedPointer = nullptr;

    std::unique_ptr<Texture> m_colorAttachmentTexture = nullptr;
    std::unique_ptr<TextureView> m_colorAttachmentTextureView = nullptr;
    std::unique_ptr<Texture> m_depthStencilTexture = nullptr;
    std::unique_ptr<TextureView> m_depthStencilTextureView = nullptr;

    std::vector<std::unique_ptr<BindGroupLayout>> m_bindGroupLayouts{};
    std::vector<std::unique_ptr<BindGroup>> m_bindGroups{};

    std::unique_ptr<PipelineLayout> m_pipelineLayout = nullptr;
    std::unique_ptr<RenderPipeline> m_renderPipeline = nullptr;

    std::unique_ptr<ShaderModule> m_vertexShaderModule = nullptr;
    std::unique_ptr<ShaderModule> m_fragmentShaderModule = nullptr;

    uint32_t m_sampleCount = 1;
};

OBJModelSample::OBJModelSample(const SampleDescriptor& descriptor)
    : NativeSample(descriptor)
{
}

OBJModelSample::~OBJModelSample()
{
    m_vertexShaderModule.reset();
    m_fragmentShaderModule.reset();

    m_renderPipeline.reset();
    m_pipelineLayout.reset();

    m_bindGroupLayouts.clear();
    m_bindGroups.clear();

    m_depthStencilTextureView.reset();
    m_depthStencilTexture.reset();
    m_colorAttachmentTextureView.reset();
    m_colorAttachmentTexture.reset();

    // unmap m_uniformBufferMappedPointer;
    m_uniformBuffer.reset();

    m_imageSampler.reset();
    m_imageTextureView.reset();
    m_imageTexture.reset();

    m_indexBuffer.reset();
    m_vertexBuffer.reset();
}

void OBJModelSample::init()
{
    NativeSample::init();

    createHPCWatcher();

    // create buffer
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();

    createImageTexture();
    createImageTextureView();
    createImageSampler();

    createColorAttachmentTexture();
    createColorAttachmentTextureView();
    createDepthStencilTexture();
    createDepthStencilTextureView();

    createBindGroupLayout();
    createBindGroup();

    createPipelineLayout();
    createRenderPipeline();
}

void OBJModelSample::onUpdate()
{
    NativeSample::onUpdate();

    updateUniformBuffer();
    updateImGui();
}

void OBJModelSample::updateImGui()
{
    recordImGui({ [&]() {
        profilingWindow();
    } });
}

void OBJModelSample::onDraw()
{
    auto renderView = m_swapchain->acquireNextTextureView();

    CommandEncoderDescriptor commandEncoderDescriptor{};
    std::unique_ptr<CommandEncoder> commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);

    std::vector<ColorAttachment> colorAttachments{}; // in currently. use only one.
    colorAttachments.push_back({ .renderView = m_sampleCount > 1 ? m_colorAttachmentTextureView.get() : renderView,
                                 .resolveView = m_sampleCount > 1 ? renderView : nullptr,
                                 .loadOp = LoadOp::kClear,
                                 .storeOp = StoreOp::kStore,
                                 .clearValue = { 0.0, 0.0, 0.0, 0.0 } });
    DepthStencilAttachment depthStencilAttachment{ .textureView = m_depthStencilTextureView.get(),
                                                   .depthLoadOp = LoadOp::kClear,
                                                   .depthStoreOp = StoreOp::kStore,
                                                   .stencilLoadOp = LoadOp::kDontCare,
                                                   .stencilStoreOp = StoreOp::kDontCare,
                                                   .clearValue = { .depth = 1.0f, .stencil = 0 } };

    RenderPassEncoderDescriptor renderPassDescriptor{ .colorAttachments = colorAttachments,
                                                      .depthStencilAttachment = depthStencilAttachment };

    std::unique_ptr<RenderPassEncoder> renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
    renderPassEncoder->setPipeline(m_renderPipeline.get());
    renderPassEncoder->setBindGroup(0, m_bindGroups[0].get());
    renderPassEncoder->setBindGroup(1, m_bindGroups[1].get());
    renderPassEncoder->setVertexBuffer(0, m_vertexBuffer.get());
    renderPassEncoder->setIndexBuffer(m_indexBuffer.get(), IndexFormat::kUint16);
    renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1); // set viewport state.
    renderPassEncoder->setScissor(0, 0, m_width, m_height);        // set scissor state.
    renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_polygon.indices.size()), 1, 0, 0, 0);
    renderPassEncoder->end();

    drawImGui(commandEncoder.get(), renderView);

    auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});

    m_queue->submit({ commandBuffer.get() });
    m_swapchain->present();
}

void OBJModelSample::createVertexBuffer()
{
    // load obj as buffer for android.
    std::vector<char> buffer = utils::readFile(m_appDir / "viking_room.obj", m_handle);
    m_polygon = loadOBJ(buffer.data(), buffer.size());

    uint64_t vertexSize = static_cast<uint64_t>(sizeof(Vertex) * m_polygon.vertices.size());
    BufferDescriptor vertexStagingBufferDescriptor{ .size = vertexSize,
                                                    .usage = BufferUsageFlagBits::kCopySrc };
    std::unique_ptr<Buffer> vertexStagingBuffer = m_device->createBuffer(vertexStagingBufferDescriptor);

    void* mappedPointer = vertexStagingBuffer->map();
    memcpy(mappedPointer, m_polygon.vertices.data(), vertexSize);

    BufferDescriptor vertexBufferDescriptor{ .size = vertexSize,
                                             .usage = BufferUsageFlagBits::kVertex | BufferUsageFlagBits::kCopyDst };
    m_vertexBuffer = m_device->createBuffer(vertexBufferDescriptor);

    // copy staging buffer to target buffer
    copyBufferToBuffer(*vertexStagingBuffer, *m_vertexBuffer);

    // unmap staging buffer.
    // vertexStagingBuffer->unmap(); // TODO: need unmap before destroy it?
}

void OBJModelSample::createIndexBuffer()
{
    uint64_t indexSize = static_cast<uint64_t>(sizeof(uint16_t) * m_polygon.indices.size());
    BufferDescriptor indexBufferDescriptor{ .size = indexSize,
                                            .usage = BufferUsageFlagBits::kIndex };

    m_indexBuffer = m_device->createBuffer(indexBufferDescriptor);

    void* mappedPointer = m_indexBuffer->map();
    memcpy(mappedPointer, m_polygon.indices.data(), indexSize);
    m_indexBuffer->unmap();
}

void OBJModelSample::createUniformBuffer()
{
    BufferDescriptor descriptor{ .size = sizeof(UniformBufferObject),
                                 .usage = BufferUsageFlagBits::kUniform };
    m_uniformBuffer = m_device->createBuffer(descriptor);
    m_uniformBufferMappedPointer = m_uniformBuffer->map();
}

void OBJModelSample::createImageTexture()
{
    // load as buffer for android.
    std::vector<char> buffer = utils::readFile(m_appDir / "viking_room.png", m_handle);
    m_image = std::make_unique<Image>(buffer.data(), buffer.size());

    unsigned char* pixels = static_cast<unsigned char*>(m_image->getPixels());
    uint32_t width = m_image->getWidth();
    uint32_t height = m_image->getHeight();
    uint32_t channel = m_image->getChannel();
    uint64_t imageSize = sizeof(unsigned char) * width * height * channel;

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    // create image staging buffer.
    BufferDescriptor descriptor{ .size = imageSize, .usage = BufferUsageFlagBits::kCopySrc };
    std::unique_ptr<Buffer> imageTextureStagingBuffer = m_device->createBuffer(descriptor);

    void* mappedPointer = imageTextureStagingBuffer->map();
    memcpy(mappedPointer, pixels, imageSize);

    // create texture.
    TextureDescriptor textureDescriptor{ .type = TextureType::k2D,
                                         .format = TextureFormat::kRGBA8UnormSrgb,
                                         .usage = TextureUsageFlagBits::kCopyDst |
                                                  TextureUsageFlagBits::kTextureBinding,
                                         .width = width,
                                         .height = height,
                                         .depth = 1,
                                         .mipLevels = mipLevels,
                                         .sampleCount = 1 };
    m_imageTexture = m_device->createTexture(textureDescriptor);

    // copy image staging buffer to texture
    copyBufferToTexture(*imageTextureStagingBuffer, *m_imageTexture);

    // unmap staging buffer
    // imageTextureStagingBuffer->unmap(); // TODO: need unmap before destroy it?
}

void OBJModelSample::createImageTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_imageTextureView = m_imageTexture->createTextureView(descriptor);
}

void OBJModelSample::createColorAttachmentTexture()
{
    // create color texture.
    TextureDescriptor textureDescriptor{ .type = TextureType::k2D,
                                         .format = m_swapchain->getTextureFormat(),
                                         .usage = TextureUsageFlagBits::kRenderAttachment,
                                         .width = m_swapchain->getWidth(),
                                         .height = m_swapchain->getHeight(),
                                         .depth = 1,
                                         .mipLevels = 1,
                                         .sampleCount = m_sampleCount };
    m_colorAttachmentTexture = m_device->createTexture(textureDescriptor);
}

void OBJModelSample::createColorAttachmentTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_colorAttachmentTextureView = m_colorAttachmentTexture->createTextureView(descriptor);
}

void OBJModelSample::createDepthStencilTexture()
{
    TextureDescriptor descriptor{};
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kDepth32Float;
    descriptor.usage = TextureUsageFlagBits::kRenderAttachment;
    descriptor.mipLevels = 1;
    descriptor.width = m_swapchain->getWidth();
    descriptor.height = m_swapchain->getHeight();
    descriptor.depth = 1;
    descriptor.sampleCount = m_sampleCount;

    m_depthStencilTexture = m_device->createTexture(descriptor);
}

void OBJModelSample::createDepthStencilTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kDepth;

    m_depthStencilTextureView = m_depthStencilTexture->createTextureView(descriptor);
}

void OBJModelSample::createImageSampler()
{
    SamplerDescriptor descriptor{};
    descriptor.magFilter = FilterMode::kLinear;
    descriptor.minFilter = FilterMode::kLinear;
    descriptor.mipmapFilter = MipmapFilterMode::kLinear;
    descriptor.addressModeU = AddressMode::kClampToEdge;
    descriptor.addressModeV = AddressMode::kClampToEdge;
    descriptor.addressModeW = AddressMode::kClampToEdge;
    descriptor.lodMin = 0.0f;
    // descriptor.lodMin = static_cast<float>(m_imageTexture->getMipLevels() / 2);
    descriptor.lodMax = static_cast<float>(m_imageTexture->getMipLevels());

    m_imageSampler = m_device->createSampler(descriptor);
}

void OBJModelSample::createBindGroupLayout()
{
    m_bindGroupLayouts.resize(2);
    {
        // Uniform Buffer
        BufferBindingLayout bufferBindingLayout{};
        bufferBindingLayout.type = BufferBindingType::kUniform;
        bufferBindingLayout.index = 0;
        bufferBindingLayout.stages = BindingStageFlagBits::kVertexStage;
        std::vector<BufferBindingLayout> bufferBindingLayouts{ bufferBindingLayout };

        BindGroupLayoutDescriptor bindGroupLayoutDescriptor{ .buffers = bufferBindingLayouts };

        m_bindGroupLayouts[0] = m_device->createBindGroupLayout(bindGroupLayoutDescriptor);
    }
    {
        // Sampler
        SamplerBindingLayout samplerBindingLayout{};
        samplerBindingLayout.index = 0;
        samplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

        std::vector<SamplerBindingLayout> samplerBindingLayouts{ samplerBindingLayout };

        // Texture
        TextureBindingLayout textureBindingLayout{};
        textureBindingLayout.index = 1;
        textureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

        std::vector<TextureBindingLayout> textureBindingLayouts{ textureBindingLayout };

        BindGroupLayoutDescriptor bindGroupLayoutDescriptor{ .buffers = {},
                                                             .samplers = samplerBindingLayouts,
                                                             .textures = textureBindingLayouts };

        m_bindGroupLayouts[1] = m_device->createBindGroupLayout(bindGroupLayoutDescriptor);
    }
}

void OBJModelSample::createBindGroup()
{
    m_bindGroups.resize(2);
    {
        BufferBinding bufferBinding{
            .index = 0,
            .offset = 0,
            .size = sizeof(UniformBufferObject),
            .buffer = m_uniformBuffer.get(),
        };

        BindGroupDescriptor descriptor{
            .layout = m_bindGroupLayouts[0].get(),
            .buffers = { bufferBinding },
        };

        m_bindGroups[0] = m_device->createBindGroup(descriptor);
    }

    {
        SamplerBinding samplerBinding{
            .index = 0,
            .sampler = m_imageSampler.get(),
        };

        TextureBinding textureBinding{
            .index = 1,
            .textureView = m_imageTextureView.get(),
        };

        BindGroupDescriptor descriptor{
            .layout = m_bindGroupLayouts[1].get(),
            .samplers = { samplerBinding },
            .textures = { textureBinding },
        };

        m_bindGroups[1] = m_device->createBindGroup(descriptor);
    }
}

void OBJModelSample::createPipelineLayout()
{
    PipelineLayoutDescriptor pipelineLayoutDescriptor{ .layouts = { m_bindGroupLayouts[0].get(), m_bindGroupLayouts[1].get() } };
    m_pipelineLayout = m_device->createPipelineLayout(pipelineLayoutDescriptor);
}

void OBJModelSample::createRenderPipeline()
{

    // Input Assembly
    InputAssemblyStage inputAssembly{};
    {
        inputAssembly.topology = PrimitiveTopology::kTriangleList;
    }

    // vertex stage
    {
        // create vertex shader
        const std::vector<char> vertShaderCode = utils::readFile(m_appDir / "obj_model.vert.spv", m_handle);
        ShaderModuleDescriptor vertexShaderModuleDescriptor{ .type = ShaderModuleType::kSPIRV,
                                                             .code = std::string_view(vertShaderCode.data(), vertShaderCode.size()) };
        m_vertexShaderModule = m_device->createShaderModule(vertexShaderModuleDescriptor);
    }

    // layouts
    std::vector<VertexInputLayout> layouts{};
    layouts.resize(1);
    {
        // attributes
        std::vector<VertexAttribute> vertexAttributes{};
        vertexAttributes.resize(2);
        {
            // position
            vertexAttributes[0] = { .format = VertexFormat::kFloat32x3,
                                    .offset = offsetof(Vertex, pos),
                                    .location = 0 };

            // texture coodinate
            vertexAttributes[1] = { .format = VertexFormat::kFloat32x2,
                                    .offset = offsetof(Vertex, texCoord),
                                    .location = 1 };
        }

        VertexInputLayout vertexLayout{ .mode = VertexMode::kVertex,
                                        .stride = sizeof(Vertex),
                                        .attributes = vertexAttributes };
        layouts[0] = vertexLayout;
    }

    VertexStage vertexStage{
        { m_vertexShaderModule.get(), "main" },
        layouts
    };

    // Rasterization
    RasterizationStage rasterization{};
    {
        rasterization.sampleCount = m_sampleCount;
        rasterization.cullMode = CullMode::kNone;
        rasterization.frontFace = FrontFace::kCounterClockwise;
    }

    // fragment stage
    {
        // create fragment shader
        const std::vector<char> fragShaderCode = utils::readFile(m_appDir / "obj_model.frag.spv", m_handle);
        ShaderModuleDescriptor fragmentShaderModuleDescriptor{};
        fragmentShaderModuleDescriptor.type = ShaderModuleType::kSPIRV;
        fragmentShaderModuleDescriptor.code = std::string_view(fragShaderCode.data(), fragShaderCode.size());
        m_fragmentShaderModule = m_device->createShaderModule(fragmentShaderModuleDescriptor);
    }

    FragmentStage fragmentStage{
        { m_fragmentShaderModule.get(), "main" },
        { { .format = m_swapchain->getTextureFormat() } }
    };

    // Depth/Stencil stage
    DepthStencilStage depthStencilStage;
    {
        depthStencilStage.format = m_depthStencilTexture->getFormat();
    }

    RenderPipelineDescriptor descriptor{
        m_pipelineLayout.get(),
        inputAssembly,
        vertexStage,
        rasterization,
        fragmentStage,
        depthStencilStage
    };

    m_renderPipeline = m_device->createRenderPipeline(descriptor);
}

void OBJModelSample::copyBufferToBuffer(Buffer& src, Buffer& dst)
{
    CopyBuffer srcBuffer{
        .buffer = &src,
        .offset = 0,
    };

    CopyBuffer dstBuffer{
        .buffer = &dst,
        .offset = 0,
    };

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);

    commandEncoder->copyBufferToBuffer(srcBuffer, dstBuffer, src.getSize());

    CommandBufferDescriptor commandBufferDescriptor{};
    auto commandBuffer = commandEncoder->finish(commandBufferDescriptor);
    m_queue->submit({ commandBuffer.get() });
}

void OBJModelSample::copyBufferToTexture(Buffer& imageTextureStagingBuffer, Texture& imageTexture)
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

    CommandBufferDescriptor commandBufferDescriptor{};
    auto commandBuffer = commandEndoer->finish(commandBufferDescriptor);
    m_queue->submit({ commandBuffer.get() });
}

void OBJModelSample::updateUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), m_swapchain->getWidth() / static_cast<float>(m_swapchain->getHeight()), 0.1f, 10.0f);

    memcpy(m_uniformBufferMappedPointer, &ubo, sizeof(ubo));
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
        { 1000, 2000, "OBJModel", app },
        ""
    };

    jipu::OBJModelSample sample(descriptor);

    sample.exec();
}

#else

int main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::trace);

    jipu::SampleDescriptor descriptor{
        { 800, 600, "OBJModel", nullptr },
        argv[0]
    };

    jipu::OBJModelSample sample(descriptor);

    return sample.exec();
}

#endif
