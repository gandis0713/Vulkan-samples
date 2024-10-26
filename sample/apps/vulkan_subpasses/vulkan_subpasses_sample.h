#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "file.h"
#include "light.h"
#include "model.h"
#include "sample.h"

#include "jipu/native/include/buffer.h"
#include "jipu/native/include/command_buffer.h"
#include "jipu/native/include/command_encoder.h"
#include "jipu/native/include/device.h"
#include "jipu/native/include/instance.h"
#include "jipu/native/include/physical_device.h"
#include "jipu/native/include/pipeline_layout.h"
#include "jipu/native/include/query_set.h"
#include "jipu/native/include/queue.h"
#include "jipu/native/include/render_pass_encoder.h"
#include "jipu/native/include/surface.h"
#include "jipu/native/include/swapchain.h"

#include "vulkan_pipeline.h"

#include "khronos_texture.h"

namespace jipu
{

class VulkanFramebuffer;
class VulkanRenderPass;
class VulkanSubpassesSample : public Sample
{
public:
    VulkanSubpassesSample() = delete;
    VulkanSubpassesSample(const SampleDescriptor& descriptor);
    ~VulkanSubpassesSample() override;

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

    void createMultipassQuerySet();
    void createSubpassQuerySet();

    VulkanRenderPass& getSubpassesRenderPass();
    VulkanRenderPass& getSubpassesCompatibleRenderPass();
    VulkanFramebuffer& getSubpassesFrameBuffer(TextureView& renderView);

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
        int lightCount = 1;
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
        } subPasses;

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

        } renderPasses;
        std::unique_ptr<Texture> colorMapTexture = nullptr;
        std::unique_ptr<TextureView> colorMapTextureView = nullptr;
        std::unique_ptr<Texture> normalMapTexture = nullptr;
        std::unique_ptr<TextureView> normalMapTextureView = nullptr;
        std::unique_ptr<Sampler> colorMapSampler = nullptr;
        std::unique_ptr<Sampler> normalMapSampler = nullptr;
        std::unique_ptr<Buffer> uniformBuffer = nullptr;
        std::unique_ptr<Buffer> vertexBuffer = nullptr;
        std::unique_ptr<Buffer> indexBuffer = nullptr;
        std::unique_ptr<Buffer> colorStagingBuffer = nullptr;
        std::unique_ptr<Buffer> normalStagingBuffer = nullptr;
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
        struct
        {
            std::vector<std::unique_ptr<BindingGroupLayout>> bindingGroupLayouts{};
            std::vector<std::unique_ptr<BindingGroup>> bindingGroups{};
            std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
            std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
            std::unique_ptr<PipelineLayout> pipelineLayout = nullptr;
            std::unique_ptr<RenderPipeline> renderPipeline = nullptr;
        } subPasses;

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

        } renderPasses;

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

    std::unique_ptr<CommandBuffer> m_commandBuffer = nullptr;
    std::unique_ptr<CommandBuffer> m_copyColorTextureCommandBuffer = nullptr;
    std::unique_ptr<CommandBuffer> m_copyNomralTextureCommandBuffer = nullptr;
    std::unique_ptr<Texture> m_depthStencilTexture = nullptr;
    std::unique_ptr<TextureView> m_depthStencilTextureView = nullptr;
    std::unique_ptr<Buffer> m_multipass1QueryBuffer = nullptr;
    std::unique_ptr<Buffer> m_multipass2QueryBuffer = nullptr;
    std::unique_ptr<Buffer> m_subpassQueryBuffer = nullptr;
    std::unique_ptr<QuerySet> m_multipass1QuerySet = nullptr;
    std::unique_ptr<QuerySet> m_multipass2QuerySet = nullptr;
    std::unique_ptr<QuerySet> m_subpassQuerySet = nullptr;

    uint32_t m_sampleCount = 1; // use only 1, because there is not resolve texture.
    int m_lightMax = 1000;
    bool m_useSubpasses = false;
    bool m_useTimestamp = false;
};

} // namespace jipu