#include "wgpu_sample.h"

#include "stanford_dragon.h"

#include <glm/glm.hpp>

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

    void createVertexBuffer();
    void createIndexBuffer();
    void createFloat16Texture();
    void createAlbedoTexture();
    void createDepthTexture();
    void createFloat16TextureView();
    void createAlbedoTextureView();
    void createDepthTextureView();
    void createShaderModules();
    void createGBufferBindGroupLayout();
    void createGBufferBindGroup();
    void createGBufferPipelineLayout();
    void createGBufferRenderPipeline();

private:
    WGPUBuffer m_vertexBuffer = nullptr;
    WGPUBuffer m_indexBuffer = nullptr;
    WGPUTexture m_float16Texture = nullptr;
    WGPUTexture m_albedoTexture = nullptr;
    WGPUTexture m_depthTexture = nullptr;
    WGPUTextureView m_float16TextureView = nullptr;
    WGPUTextureView m_albedoTextureView = nullptr;
    WGPUTextureView m_depthTextureView = nullptr;
    WGPUShaderModule m_fragmentDeferredRenderingShaderModule = nullptr;
    WGPUShaderModule m_fragmentGBufferDebugViewShaderModule = nullptr;
    WGPUShaderModule m_fragmentWriteGBuffersShaderModule = nullptr;
    WGPUShaderModule m_vertexTextureQuadShaderModule = nullptr;
    WGPUShaderModule m_vertexWriteGBuffersShaderModule = nullptr;
    WGPUShaderModule m_lightUpdateShaderModule = nullptr;
    WGPUBindGroupLayout m_gBufferBindGroupLayout = nullptr;
    WGPUBindGroup m_gBufferBindGroup = nullptr;
    WGPUPipelineLayout m_gBufferPipelineLayout = nullptr;
    WGPURenderPipeline m_gBufferRenderPipeline = nullptr;

private:
    int m_mode = 0;
    int m_numLights = 24;

    glm::vec3 m_lightExtentMin{ -50.f, -30.f, -50.f };
    glm::vec3 m_lightExtentMax{ 50.f, 50.f, 50.f };

    StanfordDragonMesh m_dragonMesh;
};

} // namespace jipu