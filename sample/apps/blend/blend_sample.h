#include "camera.h"
#include "file.h"
#include "image.h"
#include "sample.h"

#include "jipu/native/adapter.h"
#include "jipu/native/buffer.h"
#include "jipu/native/command_buffer.h"
#include "jipu/native/command_encoder.h"
#include "jipu/native/device.h"
#include "jipu/native/physical_device.h"
#include "jipu/native/pipeline.h"
#include "jipu/native/pipeline_layout.h"
#include "jipu/native/queue.h"
#include "jipu/native/surface.h"
#include "jipu/native/swapchain.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

class BlendSample : public Sample
{
public:
    BlendSample() = delete;
    BlendSample(const SampleDescriptor& descriptor);
    ~BlendSample() override;

    void init() override;
    void update() override;
    void draw() override;

private:
    void updateImGui();

private:
    void createVertexBuffer();
    void createIndexBuffer();
    void createSampler();
    std::unique_ptr<Texture> createTexture(const char* name);
    std::unique_ptr<TextureView> createTextureView(Texture* texture);
    void createBindGroupLayout();
    std::unique_ptr<BindGroup> createBindGroup(TextureView* textureView);
    void createRenderPipelineLayout();
    std::unique_ptr<RenderPipeline> createRenderPipeline(const BlendState& blendState);

    void copyBufferToTexture(Buffer& imageTextureStagingBuffer, Texture& imageTexture);

private:
    std::unique_ptr<Buffer> m_vertexBuffer = nullptr;
    std::unique_ptr<Buffer> m_indexBuffer = nullptr;
    std::unique_ptr<Texture> m_texture1 = nullptr;
    std::unique_ptr<Texture> m_texture2 = nullptr;
    std::unique_ptr<TextureView> m_textureView1 = nullptr;
    std::unique_ptr<TextureView> m_textureView2 = nullptr;
    std::unique_ptr<Sampler> m_sampler = nullptr;

    std::unique_ptr<BindGroupLayout> m_bindGroupLayout = nullptr;
    std::unique_ptr<BindGroup> m_bindGroup1 = nullptr;
    std::unique_ptr<BindGroup> m_bindGroup2 = nullptr;
    std::unique_ptr<PipelineLayout> m_renderPipelineLayout = nullptr;
    std::unique_ptr<RenderPipeline> m_renderPipeline1 = nullptr;
    std::unique_ptr<RenderPipeline> m_renderPipeline2 = nullptr;
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec2 texCoord;
    };
    std::vector<uint16_t> m_indices{ 0, 1, 2, 0, 2, 3 };
    // clang-format off
    std::vector<Vertex> m_vertices{
            { { 1.0, 1.0, }, { 1.0, 1.0, } },
            { { 0.0, 1.0, }, { 0.0, 1.0, } },
            { { 0.0, -1.0, }, { 0.0, 0.0, } },
            { { 1.0, -1.0, }, { 1.0, 0.0, } },
        };
    // clang-format on

    uint32_t m_sampleCount = 1; // use only 1, because there is not resolve texture.

    std::unique_ptr<Image> m_image1 = nullptr;
    std::unique_ptr<Image> m_image2 = nullptr;

    uint32_t m_blendColorSrcFactor = static_cast<uint32_t>(BlendFactor::kOne);
    uint32_t m_blendColorDstFactor = static_cast<uint32_t>(BlendFactor::kZero);
    uint32_t m_blendColorOp = static_cast<uint32_t>(BlendOperation::kAdd);
    uint32_t m_blendAlphaSrcFactor = static_cast<uint32_t>(BlendFactor::kOne);
    uint32_t m_blendAlphaDstFactor = static_cast<uint32_t>(BlendFactor::kZero);
    uint32_t m_blendAlphaOp = static_cast<uint32_t>(BlendOperation::kAdd);
};

} // namespace jipu