#include "camera.h"
#include "file.h"
#include "native_sample.h"

#include "jipu/native/adapter.h"
#include "jipu/native/buffer.h"
#include "jipu/native/command_buffer.h"
#include "jipu/native/command_encoder.h"
#include "jipu/native/device.h"
#include "jipu/native/physical_device.h"
#include "jipu/native/pipeline.h"
#include "jipu/native/pipeline_layout.h"
#include "jipu/native/query_set.h"
#include "jipu/native/queue.h"
#include "jipu/native/surface.h"
#include "jipu/native/swapchain.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

class QuerySample : public NativeSample
{
public:
    QuerySample() = delete;
    QuerySample(const SampleDescriptor& descriptor);
    ~QuerySample() override;

    void init() override;
    void onUpdate() override;
    void onDraw() override;

private:
    void updateImGui();

private:
    void createCamera();

    void updateUniformBuffer();

private:
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();
    void createBindGroupLayout();
    void createBindGroup();
    void createRenderPipeline();
    void createQuerySet();

private:
    std::unique_ptr<Buffer> m_vertexBuffer = nullptr;
    std::unique_ptr<Buffer> m_indexBuffer = nullptr;
    std::unique_ptr<Buffer> m_uniformBuffer = nullptr;
    std::unique_ptr<Buffer> m_timestampQueryBuffer = nullptr; // use same buffer with occlusion
    std::unique_ptr<Buffer> m_occlusionQueryBuffer = nullptr; // use same buffer with timestamp
    std::unique_ptr<BindGroupLayout> m_bindGroupLayout = nullptr;
    std::unique_ptr<BindGroup> m_bindGroup = nullptr;
    std::unique_ptr<PipelineLayout> m_renderPipelineLayout = nullptr;
    std::unique_ptr<RenderPipeline> m_renderPipeline = nullptr;
    std::unique_ptr<QuerySet> m_timestampQuerySet = nullptr;
    std::unique_ptr<QuerySet> m_occlusionQuerySet = nullptr;

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

    uint32_t m_sampleCount = 1; // use only 1, because there is not resolve texture.
    std::unique_ptr<Camera> m_camera = nullptr;
    bool m_useTimestamp = false;
    bool m_useOcclusion = false;
};

} // namespace jipu