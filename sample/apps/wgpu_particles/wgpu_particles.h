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
    void createPipelineLayout();
    void createPipeline();

private:
    WGPUPipelineLayout m_pipelineLayout = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;
    WGPUShaderModule m_vertSPIRVShaderModule = nullptr;
    WGPUShaderModule m_fragSPIRVShaderModule = nullptr;
    WGPUShaderModule m_vertWGSLShaderModule = nullptr;
    WGPUShaderModule m_fragWGSLShaderModule = nullptr;
};

} // namespace jipu