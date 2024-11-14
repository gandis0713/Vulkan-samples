#include "file.h"
#include "image.h"
#include "model.h"
#include "sample.h"

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

#include "vulkan_surface.h"
#include "vulkan_swapchain.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <spdlog/spdlog.h>
#include <stddef.h>

namespace jipu
{

class VulkanNBufferingSample : public Sample
{
public:
    VulkanNBufferingSample() = delete;
    VulkanNBufferingSample(const SampleDescriptor& descriptor);
    ~VulkanNBufferingSample() override;

    void init() override;
    void update() override;
    void draw() override;

private:
    void updateImGui();

private:
    void createSurface() override;
    void createSwapchain() override;

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

    void recreateSwapchain();

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

    std::unique_ptr<CommandBuffer> m_renderCommandBuffer = nullptr;

    uint32_t m_sampleCount = 1;

    VulkanSurfaceInfo m_surfaceInfo{};
    VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t m_minImageCount = 2;
};

} // namespace jipu