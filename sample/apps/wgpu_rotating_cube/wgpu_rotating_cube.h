#include "wgpu_sample.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace jipu
{

class WGPURotatingCube : public WGPUSample
{
public:
    WGPURotatingCube() = delete;
    WGPURotatingCube(const WGPUSampleDescriptor& descriptor);
    ~WGPURotatingCube() override;

    void init() override;
    void update() override;
    void draw() override;

    void initializeContext() override;
    void finalizeContext() override;

    void createCubeBuffer();
    void createShaderModule();
    void createPipelineLayout();
    void createPipeline();

private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    // TODO: is Dawn only supports 4 bytes alignment for mapped pointer???
    //       if so, we need to copy the index data to the buffer in 4 bytes alignment.
    //       offset should be 0, 4, 8, 12, 16, 20, ...
    //       size should be 4, 8..
    // std::vector<uint16_t> m_indices{ 0, 1, 2 };
    std::vector<uint32_t> m_indices{ 0, 1, 2 };
    std::vector<Vertex>
        m_vertices{
            { { 0.0, -1.0, 0.0 }, { 1.0, 0.0, 0.0 } },
            { { -1.0, 1.0, 0.0 }, { 0.0, 1.0, 0.0 } },
            { { 1.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 } },
        };

private:
    WGPUBuffer m_cubeVertexBuffer = nullptr;
    WGPUBuffer m_cubeIndexBuffer = nullptr;

    WGPUPipelineLayout m_pipelineLayout = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;
    WGPUShaderModule m_vertWGSLShaderModule = nullptr;
    WGPUShaderModule m_fragWGSLShaderModule = nullptr;
};

} // namespace jipu