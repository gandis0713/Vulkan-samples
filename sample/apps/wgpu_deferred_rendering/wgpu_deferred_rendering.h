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
    void createModelUniformBuffer();
    void createCameraUniformBuffer();
    void createConfigUniformBuffer();
    void createLightBuffer();
    void createLightExtentBuffer();
    void createFloat16Texture();
    void createAlbedoTexture();
    void createDepthTexture();
    void createFloat16TextureView();
    void createAlbedoTextureView();
    void createDepthTextureView();

    void createShaderModules();

    void createGBufferWriteBindGroupLayout();
    void createGBufferWriteBindGroup();
    void createGBufferWritePipelineLayout();
    void createGBufferWriteRenderPipeline();

    void createLightBufferBindGroupLayout();
    void createLightBufferBindGroup();
    void createLightBufferComputeBindGroupLayout();
    void createLightBufferComputeBindGroup();
    void createLightPipelineLayout();
    void createLightComputePipeline();

    void createGBufferTextureBindGroupLayout();
    void createGBufferTextureBindGroup();

private:
    struct ModelUniform
    {
        glm::mat4 model;
        glm::mat4 modelNormal;
    };

    struct CameraUniform
    {
        glm::mat4 viewMatrix;
        glm::mat4 invViewMatrix;
    };

private:
    WGPUBuffer m_vertexBuffer = nullptr;
    WGPUBuffer m_indexBuffer = nullptr;
    WGPUBuffer m_modelUniformBuffer = nullptr;
    WGPUBuffer m_cameraUniformBuffer = nullptr;
    WGPUBuffer m_configUniformBuffer = nullptr;
    WGPUBuffer m_lightBuffer = nullptr;
    WGPUBuffer m_lightExtentBuffer = nullptr;
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

    WGPUBindGroupLayout m_gBufferWriteBindGroupLayout = nullptr;
    WGPUBindGroup m_gBufferWriteBindGroup = nullptr;
    WGPUPipelineLayout m_gBufferWritePipelineLayout = nullptr;
    WGPURenderPipeline m_gBufferWriteRenderPipeline = nullptr;

    WGPUBindGroupLayout m_lightBufferBindGroupLayout = nullptr;
    WGPUBindGroup m_lightBufferBindGroup = nullptr;
    WGPUBindGroupLayout m_lightBufferComputeBindGroupLayout = nullptr;
    WGPUBindGroup m_lightBufferComputeBindGroup = nullptr;
    WGPUPipelineLayout m_lightPipelineLayout = nullptr;
    WGPUComputePipeline m_lightComputePipeline = nullptr;

    WGPUBindGroupLayout m_gBufferTextureBindGroupLayout = nullptr;
    WGPUBindGroup m_gBufferTextureBindGroup = nullptr;

private:
    int m_mode = 0;
    int m_numLights = 24;

    glm::vec3 m_lightExtentMin{ -50.f, -30.f, -50.f };
    glm::vec3 m_lightExtentMax{ 50.f, 50.f, 50.f };

    StanfordDragonMesh m_dragonMesh;
};

} // namespace jipu