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

    void createShaderModule();
    void createRenderPipelineLayout();
    void createRenderPipeline();
    void createComputePipelineLayout();
    void createComputePipeline();

private:
    WGPURenderPipeline m_renderPipeline = nullptr;
    WGPUComputePipeline m_computePipeline = nullptr;
    WGPUPipelineLayout m_renderPipelineLayout = nullptr;
    WGPUPipelineLayout m_computePipelineLayout = nullptr;
    WGPUShaderModule m_wgslParticleShaderModule = nullptr;
    WGPUShaderModule m_wgslProbablilityMapShaderModule = nullptr;
};

} // namespace jipu