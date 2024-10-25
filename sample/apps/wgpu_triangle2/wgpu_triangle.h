#include "wgpu_sample.h"

#if defined(USE_DAWN_HEADER)
#include <dawn/webgpu.h>
#else
#include <webgpu.h>
#endif

namespace jipu
{

class WGPUTriangleSample : public WGPUSample
{
public:
    WGPUTriangleSample() = delete;
    WGPUTriangleSample(const WGPUSampleDescriptor& descriptor);
    ~WGPUTriangleSample() override;

    void init() override;
    void update() override;
    void draw() override;

    void createInstance();
    void createSurface();
    void createAdapter();
    void createDevice();
    void createSurfaceConfigure();
    void createQueue();
    void createShaderModule();
    void createPipelineLayout();
    void createPipeline();

private:
    WGPUInstance m_instance = nullptr;

    WGPUSurface m_surface = nullptr;
    WGPUSurfaceCapabilities m_surfaceCapabilities{};
    WGPUSurfaceConfiguration m_surfaceConfigure{};

    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;

    WGPUQueue m_queue = nullptr;

    WGPUPipelineLayout m_pipelineLayout = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;
    [[maybe_unused]] WGPUShaderModule m_vertSPIRVShaderModule = nullptr;
    [[maybe_unused]] WGPUShaderModule m_fragSPIRVShaderModule = nullptr;
    [[maybe_unused]] WGPUShaderModule m_vertWGSLShaderModule = nullptr;
    [[maybe_unused]] WGPUShaderModule m_fragWGSLShaderModule = nullptr;
};

} // namespace jipu