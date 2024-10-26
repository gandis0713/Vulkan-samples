#include "camera.h"
#include "file.h"
#include "sample.h"

#include "jipu/native/include/buffer.h"
#include "jipu/native/include/command_buffer.h"
#include "jipu/native/include/command_encoder.h"
#include "jipu/native/include/device.h"
#include "jipu/native/include/instance.h"
#include "jipu/native/include/physical_device.h"
#include "jipu/native/include/pipeline.h"
#include "jipu/native/include/pipeline_layout.h"
#include "jipu/native/include/query_set.h"
#include "jipu/native/include/queue.h"
#include "jipu/native/include/surface.h"
#include "jipu/native/include/swapchain.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace jipu
{

class QuerySample : public Sample
{
public:
    QuerySample() = delete;
    QuerySample(const SampleDescriptor& descriptor);
    ~QuerySample() override;

    void init() override;
    void update() override;
    void draw() override;

private:
    void updateImGui();

private:
    void createCamera();

    void updateUniformBuffer();

private:
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();
    void createBindingGroupLayout();
    void createBindingGroup();
    void createRenderPipeline();
    void createQuerySet();

private:
    std::unique_ptr<Buffer> m_vertexBuffer = nullptr;
    std::unique_ptr<Buffer> m_indexBuffer = nullptr;
    std::unique_ptr<Buffer> m_uniformBuffer = nullptr;
    std::unique_ptr<Buffer> m_timestampQueryBuffer = nullptr; // use same buffer with occlusion
    std::unique_ptr<Buffer> m_occlusionQueryBuffer = nullptr; // use same buffer with timestamp
    std::unique_ptr<BindingGroupLayout> m_bindingGroupLayout = nullptr;
    std::unique_ptr<BindingGroup> m_bindingGroup = nullptr;
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