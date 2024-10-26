

#include "camera.h"
#include "file.h"
#include "sample.h"

#include "jipu/native/buffer.h"
#include "jipu/native/command_buffer.h"
#include "jipu/native/command_encoder.h"
#include "jipu/native/device.h"
#include "jipu/native/instance.h"
#include "jipu/native/physical_device.h"
#include "jipu/native/pipeline.h"
#include "jipu/native/pipeline_layout.h"
#include "jipu/native/queue.h"
#include "jipu/native/surface.h"
#include "jipu/native/swapchain.h"

namespace jipu
{

class OffscreenSample : public Sample
{
public:
    OffscreenSample() = delete;
    OffscreenSample(const SampleDescriptor& descriptor);
    ~OffscreenSample() override;

    void init() override;
    void update() override;
    void draw() override;

private:
    void updateImGui();

private:
    void updateOffscreenUniformBuffer();

private:
    void createOffscreenTexture();
    void createOffscreenTextureView();
    void createOffscreenVertexBuffer();
    void createOffscreenIndexBuffer();
    void createOffscreenUniformBuffer();
    void createOffscreenBindingGroupLayout();
    void createOffscreenBindingGroup();
    void createOffscreenRenderPipeline();

    void createOnscreenVertexBuffer();
    void createOnscreenIndexBuffer();
    void createOnscreenSampler();
    void createOnscreenBindingGroupLayout();
    void createOnscreenBindingGroup();
    void createOnscreenRenderPipeline();

    void createCamera();

private:
    struct
    {
        std::unique_ptr<Texture> renderTexture = nullptr;
        std::unique_ptr<TextureView> renderTextureView = nullptr;
        std::unique_ptr<Buffer> vertexBuffer = nullptr;
        std::unique_ptr<Buffer> indexBuffer = nullptr;
        std::unique_ptr<Buffer> uniformBuffer = nullptr;
        std::unique_ptr<BindingGroupLayout> bindingGroupLayout = nullptr;
        std::unique_ptr<BindingGroup> bindingGroup = nullptr;
        std::unique_ptr<PipelineLayout> renderPipelineLayout = nullptr;
        std::unique_ptr<RenderPipeline> renderPipeline = nullptr;
    } m_offscreen;

    struct
    {
        std::unique_ptr<Buffer> vertexBuffer = nullptr;
        std::unique_ptr<Buffer> indexBuffer = nullptr;
        std::unique_ptr<Sampler> sampler = nullptr;
        std::unique_ptr<BindingGroupLayout> bindingGroupLayout = nullptr;
        std::unique_ptr<BindingGroup> bindingGroup = nullptr;
        std::unique_ptr<PipelineLayout> renderPipelineLayout = nullptr;
        std::unique_ptr<RenderPipeline> renderPipeline = nullptr;
    } m_onscreen;

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

    struct OffscreenVertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    std::vector<OffscreenVertex> m_offscreenVertices{
        { { 0.0, -500, 0.0 }, { 1.0, 0.0, 0.0 } },
        { { -500, 500, 0.0 }, { 0.0, 1.0, 0.0 } },
        { { 500, 500, 0.0 }, { 0.0, 0.0, 1.0 } },
    };
    std::vector<uint16_t> m_offscreenIndices{ 0, 1, 2 };

    struct OnscreenVertex
    {
        glm::vec3 pos;
        glm::vec2 texCoord;
    };
    std::vector<OnscreenVertex> m_onscreenVertices{
        { { -1.0, -1.0, 0.0 }, { 0.0, 0.0 } },
        { { -1.0, 1.0, 0.0 }, { 0.0, 1.0 } },
        { { 1.0, 1.0, 0.0 }, { 1.0, 1.0 } },
        { { 1.0, -1.0, 0.0 }, { 1.0, 0.0 } },
    };
    std::vector<uint16_t> m_onscreenIndices{ 0, 1, 3, 1, 2, 3 };

    uint32_t m_sampleCount = 1; // use only 1, because there is not resolve texture.
    std::unique_ptr<Camera> m_camera = nullptr;
};
} // namespace jipu