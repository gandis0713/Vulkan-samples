#include "wgpu_sample.h"

namespace jipu
{

class WGPUParticlesSample : public WGPUSample
{
public:
    WGPUParticlesSample() = delete;
    WGPUParticlesSample(const WGPUSampleDescriptor& descriptor);
    ~WGPUParticlesSample() override;

    void init() override;
    void onUpdate() override;
    void onDraw() override;

    void initializeContext() override;
    void finalizeContext() override;

    void createParticleBuffer();
    void createUniformBuffer();
    void createDepthTexture();
    void createParticleShaderModule();
    void createProbabilityMapShaderModule();
    void createProbabilityMapImportLevelPipeline();
    void createProbabilityMapImportLevelPipelineLayout();
    void createProbabilityMapExportLevelPipeline();
    void createProbabilityMapExportLevelPipelineLayout();
    void createRenderPipelineLayout();
    void createRenderPipeline();
    void createComputePipelineLayout();
    void createComputePipeline();

private:
    WGPUBuffer m_particleBuffer = nullptr;
    WGPUBuffer m_uniformBuffer = nullptr;
    WGPUTexture m_depthTexture = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;
    WGPUPipelineLayout m_renderPipelineLayout = nullptr;
    WGPUComputePipeline m_computePipeline = nullptr;
    WGPUPipelineLayout m_computePipelineLayout = nullptr;
    WGPUComputePipeline m_probabilityMapImportLevelPipeline = nullptr;
    WGPUPipelineLayout m_probabilityMapImportLevelPipelineLayout = nullptr;
    WGPUComputePipeline m_probabilityMapExportLevelPipeline = nullptr;
    WGPUPipelineLayout m_probabilityMapExportLevelPipelineLayout = nullptr;
    WGPUShaderModule m_wgslParticleShaderModule = nullptr;
    WGPUShaderModule m_wgslProbablilityMapShaderModule = nullptr;

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
};

} // namespace jipu