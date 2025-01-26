#include "wgpu_sample.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace jipu
{

class WGPUDeferredRenderingSample : public WGPUSample
{
public:
    WGPUDeferredRenderingSample() = delete;
    WGPUDeferredRenderingSample(const WGPUSampleDescriptor& descriptor);
    ~WGPUDeferredRenderingSample() override;

    void init() override;
    void onBeforeUpdate() override;
    void onUpdate() override;
    void onDraw() override;

    void initializeContext() override;
    void finalizeContext() override;

    void createQuadBuffer();
    void createParticleBuffer();
    void createUniformBuffer();
    void createSimulationUBOBuffer();
    void createDepthTexture();
    void createImageTexture();
    void createImageTextureView();
    void createParticleShaderModule();
    void createProbabilityMapShaderModule();
    void createProbabilityMapUBOBuffer();
    void createProbabilityMapBufferA();
    void createProbabilityMapBufferB();
    void createProbabilityMapImportLevelBindGroupLayout();
    void createProbabilityMapExportLevelBindGroupLayout();
    void createProbabilityMapImportLevelPipelineLayout();
    void createProbabilityMapImportLevelPipeline();
    void createProbabilityMapExportLevelPipelineLayout();
    void createProbabilityMapExportLevelPipeline();
    void generateProbabilityMap();
    void createComputeBindGroupLayout();
    void createComputeBindGroup();
    void createUniformBindGroupLayout();
    void createUniformBindGroup();
    void createComputePipelineLayout();
    void createComputePipeline();
    void createRenderPipelineLayout();
    void createRenderPipeline();

private:
    WGPUBuffer m_quadBuffer = nullptr;
    WGPUBuffer m_particleBuffer = nullptr;
    WGPUBuffer m_uniformBuffer = nullptr;
    WGPUBuffer m_simulationUBOBuffer = nullptr;
    WGPUTexture m_depthTexture = nullptr;
    WGPUTexture m_imageTexture = nullptr;
    WGPUTextureView m_imageTextureView = nullptr;
    WGPUShaderModule m_wgslParticleShaderModule = nullptr;
    WGPUShaderModule m_wgslProbablilityMapShaderModule = nullptr;
    WGPUBuffer m_probabilityMapUBOBuffer = nullptr;
    WGPUBuffer m_probabilityMapBufferA = nullptr;
    WGPUBuffer m_probabilityMapBufferB = nullptr;
    WGPUBindGroupLayout m_probabilityMapImportLevelBindGroupLayout = nullptr;
    WGPUBindGroupLayout m_probabilityMapExportLevelBindGroupLayout = nullptr;
    WGPUPipelineLayout m_probabilityMapImportLevelPipelineLayout = nullptr;
    WGPUComputePipeline m_probabilityMapImportLevelPipeline = nullptr;
    WGPUPipelineLayout m_probabilityMapExportLevelPipelineLayout = nullptr;
    WGPUComputePipeline m_probabilityMapExportLevelPipeline = nullptr;
    WGPUBindGroupLayout m_computeBindGroupLayout = nullptr;
    WGPUBindGroup m_computeBindGroup = nullptr;
    WGPUBindGroupLayout m_uniformBindGroupLayout = nullptr;
    WGPUBindGroup m_uniformBindGroup = nullptr;
    WGPUPipelineLayout m_computePipelineLayout = nullptr;
    WGPUComputePipeline m_computePipeline = nullptr;
    WGPUPipelineLayout m_renderPipelineLayout = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;

private:
    int m_numParticles = 50000;
    int m_particlePositionOffset = 0;
    int m_particleColorOffset = 4 * 4;
    int m_particleInstanceByteSize =
        3 * 4 + // position
        1 * 4 + // lifetime
        4 * 4 + // color
        3 * 4 + // velocity
        1 * 4 + // padding
        0;

    const uint32_t m_uniformBufferSize =
        4 * 4 * 4 + // modelViewProjectionMatrix : mat4x4f
        3 * 4 +     // right : vec3f
        4 +         // padding
        3 * 4 +     // up : vec3f
        4 +         // padding
        0;

    const uint32_t m_simulationUBOBufferSize =
        1 * 4 + // deltaTime
        1 * 4 + // brightnessFactor
        2 * 4 + // padding
        4 * 4 + // seed
        0;

    const uint32_t m_probabilityMapUBOBufferSize =
        1 * 4 + // stride
        3 * 4 + // padding
        0;

    uint32_t m_probabilityMapBufferSize = 0;

    uint32_t m_textureWidth = 1;
    uint32_t m_textureHeight = 1;
    uint32_t m_numMipLevels = 1;

    struct SimulationUBO
    {
        float simulateDeltaTime; // simulationParams.simulate ? simulationParams.deltaTime : 0.0
        float brightnessFactor;  // simulationParams.brightnessFactor
        float padding1;          // 0.0 (패딩)
        float padding2;          // 0.0 (패딩)
        float seedX;             // Math.random() * 100
        float seedY;             // Math.random() * 100
        float seedZ;             // 1.0 + Math.random()
        float seedW;             // 1.0 + Math.random()
    };

    struct MatrixUBO
    {
        glm::mat4 mvp;   // Model-View-Projection Matrix
        glm::vec3 right; // view[0], view[4], view[8]
        float padding1;  // 패딩
        glm::vec3 up;    // view[1], view[5], view[9]
        float padding2;  // 패딩
    };

    struct SimulationParams
    {
        bool simulate = true;
        float deltaTime = 0.04;
        // toneMappingMode : 'standard' as GPUCanvasToneMappingMode,
        float brightnessFactor = 1.0;
    } m_simulationParams;
};

} // namespace jipu