#include "wgpu_sample.h"

#if defined(USE_DAWN_HEADER)
#include <dawn/webgpu.h>
#else
#include <webgpu.h>
#endif

namespace jipu
{

class WGPUTriangleSample2 : public WGPUSample
{
public:
    WGPUTriangleSample2() = delete;
    WGPUTriangleSample2(const WGPUSampleDescriptor& descriptor);
    ~WGPUTriangleSample2() override;

    void init() override;
    void update() override;
    void draw() override;

    void initializeContext() override;
    void finalizeContext() override;

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
    struct WGPUContext
    {
        WGPUInstance instance = nullptr;

        WGPUSurface surface = nullptr;
        WGPUSurfaceCapabilities surfaceCapabilities{};
        WGPUSurfaceConfiguration surfaceConfigure{};

        WGPUAdapter adapter = nullptr;
        WGPUDevice device = nullptr;

        WGPUQueue queue = nullptr;

        WGPUPipelineLayout pipelineLayout = nullptr;
        WGPURenderPipeline renderPipeline = nullptr;
        WGPUShaderModule vertSPIRVShaderModule = nullptr;
        WGPUShaderModule fragSPIRVShaderModule = nullptr;
        WGPUShaderModule vertWGSLShaderModule = nullptr;
        WGPUShaderModule fragWGSLShaderModule = nullptr;
    } m_wgpuContext;

    WGPUContext& getContext();

private:
    bool m_useSPIRV = false;
};

} // namespace jipu