#include "wgpu_sample.h"

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

private:
    bool m_useSPIRV = false;
};

} // namespace jipu