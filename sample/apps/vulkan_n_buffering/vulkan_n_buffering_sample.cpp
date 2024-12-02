#include "vulkan_n_buffering_sample.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <optional>
#include <random>
#include <stdexcept>

#include "vulkan_adapter.h"
#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include "vulkan_surface.h"
#include "vulkan_swapchain.h"

namespace jipu
{
VulkanNBufferingSample::VulkanNBufferingSample(const SampleDescriptor& descriptor)
    : Sample(descriptor)
{
}

VulkanNBufferingSample::~VulkanNBufferingSample()
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

    // release command buffer after finising queue.
    m_renderCommandBuffer.reset();
}

void VulkanNBufferingSample::init()
{
    Sample::init();

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

void VulkanNBufferingSample::onUpdate()
{
    Sample::onUpdate();

    updateUniformBuffer();

    updateImGui();
}

void VulkanNBufferingSample::updateImGui()
{
    recordImGui({ [&]() {
        windowImGui("Settings", { [&]() {
                        auto presentModeUI = [&](const char* name, VkPresentModeKHR presentMode) {
                            auto it = std::find(m_surfaceInfo.presentModes.begin(), m_surfaceInfo.presentModes.end(), presentMode);
                            if (it == m_surfaceInfo.presentModes.end())
                                ImGui::BeginDisabled();

                            if (ImGui::RadioButton(name, m_presentMode == presentMode))
                            {
                                m_presentMode = presentMode;
                                recreateSwapchain();
                            }

                            if (it == m_surfaceInfo.presentModes.end())
                                ImGui::EndDisabled();
                        };

                        ImGui::Text("Present Mode");
                        presentModeUI("FIFO", VK_PRESENT_MODE_FIFO_KHR);
                        presentModeUI("FIFO RELAXED", VK_PRESENT_MODE_FIFO_RELAXED_KHR);
                        presentModeUI("MAILBOX", VK_PRESENT_MODE_MAILBOX_KHR);
                        presentModeUI("IMMEDIATE", VK_PRESENT_MODE_IMMEDIATE_KHR);

                        ImGui::Separator();
                        ImGui::Text("Min Image Count");

                        auto minCount = std::max(m_surfaceInfo.capabilities.minImageCount, 1u);
                        auto maxCount = std::min(m_surfaceInfo.capabilities.maxImageCount, 3u);
                        for (auto i = minCount; i <= maxCount; ++i)
                        {
                            if (ImGui::RadioButton(std::to_string(i).c_str(), m_minImageCount == i))
                            {
                                m_minImageCount = i;
                                recreateSwapchain();
                            }
                        }
                    } });
        profilingWindow();
    } });
}

void VulkanNBufferingSample::onDraw()
{
    auto renderView = m_swapchain->acquireNextTextureView();

    CommandEncoderDescriptor commandEncoderDescriptor{};
    std::unique_ptr<CommandEncoder> commandEncoder = m_renderCommandBuffer->createCommandEncoder(commandEncoderDescriptor);

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
    renderPassEncoder->setBindGroup(0, m_bindGroup.get() s[0]);
    renderPassEncoder->setBindGroup(1, *m_bindGroups[1]);
    renderPassEncoder->setVertexBuffer(0, m_vertexBuffer.get());
    renderPassEncoder->setIndexBuffer(m_indexBuffer.get(), IndexFormat::kUint16);
    renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1); // set viewport state.
    renderPassEncoder->setScissor(0, 0, m_width, m_height);        // set scissor state.
    renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_polygon.indices.size()), 1, 0, 0, 0);
    renderPassEncoder->end();

    drawImGui(commandEncoder.get(), renderView);

    m_queue->submit({ commandEncoder->finish() }, *m_swapchain);
}

void VulkanNBufferingSample::createSurface()
{
    SurfaceDescriptor descriptor{ .windowHandle = getWindowHandle() };

    VulkanSurfaceDescriptor vkdescriptor = generateVulkanSurfaceDescriptor(descriptor);

    m_surface = downcast(m_instance.get())->createSurface(vkdescriptor);

    auto vulkanSurface = downcast(m_surface.get());
    m_surfaceInfo = downcast(m_physicalDevices[0].get())->gatherSurfaceInfo(vulkanSurface);
}

void VulkanNBufferingSample::createSwapchain()
{
    if (m_surface == nullptr)
        throw std::runtime_error("Surface is null pointer.");

#if defined(__ANDROID__) || defined(ANDROID)
    TextureFormat textureFormat = TextureFormat::kRGBA8UnormSrgb;
#else
    TextureFormat textureFormat = TextureFormat::kBGRA8UnormSrgb;
#endif
    SwapchainDescriptor descriptor{
        .surface = m_surface.get(),
        .textureFormat = textureFormat,
        .presentMode = PresentMode::kFifo,
        .colorSpace = ColorSpace::kSRGBNonLinear,
        .width = m_width,
        .height = m_height
    };

    auto& vulkanDevice = downcast(*m_device);
    VulkanSwapchainDescriptor vkdescriptor = generateVulkanSwapchainDescriptor(vulkanDevice, descriptor);

    vkdescriptor.presentMode = m_presentMode;
    vkdescriptor.minImageCount = m_minImageCount;
    vkdescriptor.oldSwapchain = m_swapchain != nullptr ? downcast(m_swapchain.get())->getVkSwapchainKHR() : VK_NULL_HANDLE;

    auto swapchain = vulkanDevice.createSwapchain(vkdescriptor);

    m_swapchain = std::move(swapchain);
}

void VulkanNBufferingSample::createVertexBuffer()
{
    // load obj as buffer for android.
    std::vector<char> buffer = utils::readFile(m_appDir / "spaceship.obj", m_handle);
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

void VulkanNBufferingSample::createIndexBuffer()
{
    uint64_t indexSize = static_cast<uint64_t>(sizeof(uint16_t) * m_polygon.indices.size());
    BufferDescriptor indexBufferDescriptor{ .size = indexSize,
                                            .usage = BufferUsageFlagBits::kIndex };

    m_indexBuffer = m_device->createBuffer(indexBufferDescriptor);

    void* mappedPointer = m_indexBuffer->map();
    memcpy(mappedPointer, m_polygon.indices.data(), indexSize);
    m_indexBuffer->unmap();
}

void VulkanNBufferingSample::createUniformBuffer()
{
    BufferDescriptor descriptor{ .size = sizeof(UniformBufferObject),
                                 .usage = BufferUsageFlagBits::kUniform };
    m_uniformBuffer = m_device->createBuffer(descriptor);
    m_uniformBufferMappedPointer = m_uniformBuffer->map();
}

void VulkanNBufferingSample::createImageTexture()
{
    // load as buffer for android.
    std::vector<char> buffer = utils::readFile(m_appDir / "spaceship_color_4.jpg", m_handle);
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
                                         .usage = TextureUsageFlagBits::kCopySrc |
                                                  TextureUsageFlagBits::kCopyDst |
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

void VulkanNBufferingSample::createImageTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_imageTextureView = m_imageTexture->createTextureView(descriptor);
}

void VulkanNBufferingSample::createColorAttachmentTexture()
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

void VulkanNBufferingSample::createColorAttachmentTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kColor;

    m_colorAttachmentTextureView = m_colorAttachmentTexture->createTextureView(descriptor);
}

void VulkanNBufferingSample::createDepthStencilTexture()
{
    TextureDescriptor descriptor{};
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kDepth32Float;
    descriptor.usage = TextureUsageFlagBits::kDepthStencil;
    descriptor.mipLevels = 1;
    descriptor.width = m_swapchain->getWidth();
    descriptor.height = m_swapchain->getHeight();
    descriptor.depth = 1;
    descriptor.sampleCount = m_sampleCount;

    m_depthStencilTexture = m_device->createTexture(descriptor);
}

void VulkanNBufferingSample::createDepthStencilTextureView()
{
    TextureViewDescriptor descriptor{};
    descriptor.dimension = TextureViewDimension::k2D;
    descriptor.aspect = TextureAspectFlagBits::kDepth;

    m_depthStencilTextureView = m_depthStencilTexture->createTextureView(descriptor);
}

void VulkanNBufferingSample::createImageSampler()
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

void VulkanNBufferingSample::createBindGroupLayout()
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

void VulkanNBufferingSample::createBindGroup()
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

void VulkanNBufferingSample::createPipelineLayout()
{
    PipelineLayoutDescriptor pipelineLayoutDescriptor{ .layouts = { m_bindGroupLayouts[0].get(), m_bindGroupLayouts[1].get() } };
    m_pipelineLayout = m_device->createPipelineLayout(pipelineLayoutDescriptor);
}

void VulkanNBufferingSample::createRenderPipeline()
{

    // Input Assembly
    InputAssemblyStage inputAssembly{};
    {
        inputAssembly.topology = PrimitiveTopology::kTriangleList;
    }

    // vertex stage

    // create vertex shader
    const std::vector<char> vertShaderCode = utils::readFile(m_appDir / "vulkan_n_buffering.vert.spv", m_handle);
    ShaderModuleDescriptor vertexShaderModuleDescriptor{ .code = vertShaderCode.data(),
                                                         .codeSize = vertShaderCode.size() };
    m_vertexShaderModule = m_device->createShaderModule(vertexShaderModuleDescriptor);

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
        rasterization.cullMode = CullMode::kBack;
        rasterization.frontFace = FrontFace::kCounterClockwise;
    }

    // fragment stage

    // create fragment shader
    const std::vector<char> fragShaderCode = utils::readFile(m_appDir / "vulkan_n_buffering.frag.spv", m_handle);
    ShaderModuleDescriptor fragmentShaderModuleDescriptor{ .code = fragShaderCode.data(),
                                                           .codeSize = fragShaderCode.size() };
    m_fragmentShaderModule = m_device->createShaderModule(fragmentShaderModuleDescriptor);

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
        m_pipelineLayout.get(), // PipelineDescriptor
        inputAssembly,
        vertexStage,
        rasterization,
        fragmentStage,
        depthStencilStage,
    };

    m_renderPipeline = m_device->createRenderPipeline(descriptor);
}

void VulkanNBufferingSample::copyBufferToBuffer(Buffer& src, Buffer& dst)
{
    CopyBuffer srcBuffer{
        .buffer = &src,
        .offset = 0,
    };

    CopyBuffer dstBuffer{
        .buffer = &dst,
        .offset = 0,
    };

    CommandBufferDescriptor commandBufferDescriptor{};
    auto commandBuffer = m_device->createCommandBuffer(commandBufferDescriptor);

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = commandBuffer->createCommandEncoder(commandEncoderDescriptor);

    commandEncoder->copyBufferToBuffer(srcBuffer, dstBuffer, src.getSize());

    m_queue->submit({ commandEncoder->finish() });
}

void VulkanNBufferingSample::copyBufferToTexture(Buffer& imageTextureStagingBuffer, Texture& imageTexture)
{
    uint32_t channel = 4;                          // TODO: from texture.
    uint32_t bytesPerData = sizeof(unsigned char); // TODO: from buffer.

    CopyTextureBuffer copyTextureBuffer{
        .buffer = &imageTextureStagingBuffer,
        .offset = 0,
        .bytesPerRow = bytesPerData * imageTexture.getWidth() * channel,
        .rowsPerTexture = imageTexture.getHeight(),
    };

    CopyTexture copyTexture{ .texture = &imageTexture, .aspect = TextureAspectFlagBits::kColor };
    Extent3D extent{};
    extent.width = imageTexture.getWidth();
    extent.height = imageTexture.getHeight();
    extent.depth = 1;

    CommandBufferDescriptor commandBufferDescriptor{};
    std::unique_ptr<CommandBuffer> commandBuffer = m_device->createCommandBuffer(commandBufferDescriptor);

    CommandEncoderDescriptor commandEncoderDescriptor{};
    std::unique_ptr<CommandEncoder> commandEndoer = commandBuffer->createCommandEncoder(commandEncoderDescriptor);
    commandEndoer->copyBufferToTexture(copyTextureBuffer, copyTexture, extent);

    m_queue->submit({ commandEndoer->finish() });
}

void VulkanNBufferingSample::updateUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), m_swapchain->getWidth() / static_cast<float>(m_swapchain->getHeight()), 0.1f, 1000.0f);

    ubo.proj[1][1] *= -1;

    memcpy(m_uniformBufferMappedPointer, &ubo, sizeof(ubo));
}

void VulkanNBufferingSample::recreateSwapchain()
{
    auto vulkanDevice = downcast(m_device.get());
    vulkanDevice->vkAPI.DeviceWaitIdle(vulkanDevice->getVkDevice());
    createSwapchain();
}

} // namespace jipu
