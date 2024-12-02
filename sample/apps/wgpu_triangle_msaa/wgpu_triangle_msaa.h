#include "wgpu_sample.h"

namespace jipu
{

class WGPUTriangleSampleMSAA : public WGPUSample
{
public:
    WGPUTriangleSampleMSAA() = delete;
    WGPUTriangleSampleMSAA(const WGPUSampleDescriptor& descriptor);
    ~WGPUTriangleSampleMSAA() override;

    void init() override;
    void onUpdate() override;
    void onDraw() override;
    void onResize(uint32_t width, uint32_t height) override;

    void initializeContext() override;
    void finalizeContext() override;

    void createRenderTexture();
    void createRenderTextureView();
    void createShaderModule();
    void createRenderPipelineLayout();
    void createRenderPipeline();

    void releaseRenderTexture();
    void releaseRenderTextureView();

private:
    WGPUTexture m_renderTexture = nullptr;
    WGPUTextureView m_renderTextureView = nullptr;
    WGPUPipelineLayout m_pipelineLayout = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;
    WGPUShaderModule m_vertWGSLShaderModule = nullptr;
    WGPUShaderModule m_fragWGSLShaderModule = nullptr;

    uint32_t m_sampleCount = 4;
};

} // namespace jipu