#include "wgpu_sample.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>

namespace jipu
{

class WGPURotatingCube : public WGPUSample
{
public:
    WGPURotatingCube() = delete;
    WGPURotatingCube(const WGPUSampleDescriptor& descriptor);
    ~WGPURotatingCube() override;

    void init() override;
    void onUpdate() override;
    void onDraw() override;

    void initializeContext() override;
    void finalizeContext() override;

    void createCubeBuffer();
    void createDepthTexture();
    void createUniformBuffer();
    void createBindingGroupLayout();
    void createBindingGroup();
    void createShaderModule();
    void createPipelineLayout();
    void createPipeline();

private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    // // TODO: Dawn only supports multiple of 4.
    // using IndexType = uint16_t;
    // std::vector<IndexType> m_indices{
    //     0,
    //     1,
    //     2,
    //     0 /* padding */
    // };
    // std::vector<Vertex>
    //     m_vertices{
    //         { { 0.0, -1.0, 0.0 }, { 1.0, 0.0, 0.0 } },
    //         { { -1.0, 1.0, 0.0 }, { 0.0, 1.0, 0.0 } },
    //         { { 1.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } },
    //     };

    // clang-format off
    const std::vector<float> m_cube{
        // float4 position, float4 color, float2 uv,
        1, -1, 1, 1,   1, 0, 1, 1,  0, 1,
        -1, -1, 1, 1,  0, 0, 1, 1,  1, 1,
        -1, -1, -1, 1, 0, 0, 0, 1,  1, 0,
        1, -1, -1, 1,  1, 0, 0, 1,  0, 0,
        1, -1, 1, 1,   1, 0, 1, 1,  0, 1,
        -1, -1, -1, 1, 0, 0, 0, 1,  1, 0,

        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,
        1, -1, 1, 1,   1, 0, 1, 1,  1, 1,
        1, -1, -1, 1,  1, 0, 0, 1,  1, 0,
        1, 1, -1, 1,   1, 1, 0, 1,  0, 0,
        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,
        1, -1, -1, 1,  1, 0, 0, 1,  1, 0,

        -1, 1, 1, 1,   0, 1, 1, 1,  0, 1,
        1, 1, 1, 1,    1, 1, 1, 1,  1, 1,
        1, 1, -1, 1,   1, 1, 0, 1,  1, 0,
        -1, 1, -1, 1,  0, 1, 0, 1,  0, 0,
        -1, 1, 1, 1,   0, 1, 1, 1,  0, 1,
        1, 1, -1, 1,   1, 1, 0, 1,  1, 0,

        -1, -1, 1, 1,  0, 0, 1, 1,  0, 1,
        -1, 1, 1, 1,   0, 1, 1, 1,  1, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,
        -1, -1, -1, 1, 0, 0, 0, 1,  0, 0,
        -1, -1, 1, 1,  0, 0, 1, 1,  0, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,

        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,
        -1, 1, 1, 1,   0, 1, 1, 1,  1, 1,
        -1, -1, 1, 1,  0, 0, 1, 1,  1, 0,
        -1, -1, 1, 1,  0, 0, 1, 1,  1, 0,
        1, -1, 1, 1,   1, 0, 1, 1,  0, 0,
        1, 1, 1, 1,    1, 1, 1, 1,  0, 1,

        1, -1, -1, 1,  1, 0, 0, 1,  0, 1,
        -1, -1, -1, 1, 0, 0, 0, 1,  1, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,
        1, 1, -1, 1,   1, 1, 0, 1,  0, 0,
        1, -1, -1, 1,  1, 0, 0, 1,  0, 1,
        -1, 1, -1, 1,  0, 1, 0, 1,  1, 0,
    };
    // clang-format on

private:
    WGPUBuffer m_cubeVertexBuffer = nullptr;
    // WGPUBuffer m_cubeIndexBuffer = nullptr;
    WGPUTexture m_depthTexture = nullptr;
    WGPUBuffer m_uniformBuffer = nullptr;
    WGPUBindGroup m_bindGroup = nullptr;
    WGPUBindGroupLayout m_bindGroupLayout = nullptr;
    WGPUPipelineLayout m_pipelineLayout = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;
    WGPUShaderModule m_vertWGSLShaderModule = nullptr;
    WGPUShaderModule m_fragWGSLShaderModule = nullptr;
};

} // namespace jipu