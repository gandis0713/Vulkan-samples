#include "wgpu_sample.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>

namespace jipu
{

class WGPURenderBundles : public WGPUSample
{
public:
    WGPURenderBundles() = delete;
    WGPURenderBundles(const WGPUSampleDescriptor& descriptor);
    ~WGPURenderBundles() override;

    void init() override;
    void onBeforeUpdate() override;
    void onUpdate() override;
    void onDraw() override;

    void initializeContext() override;
    void finalizeContext() override;

    void createDepthTexture();
    void createMoonImageTexture();
    void createMoonImageTextureView();
    void createPlanetImageTexture();
    void createPlanetImageTextureView();
    void createSampler();
    void createUniformBuffer();
    void createBindingGroupLayout();
    void createBindingGroup();
    void createSphereBindGroupLayout();
    void createShaderModule();
    void createPipelineLayout();
    void createPipeline();
    void createRenderBundle();
    void renderScene(WGPURenderPassEncoder passEncoder);

public:
    struct Renderable
    {
        WGPUBuffer vertexBuffer;
        WGPUBuffer indexBuffer;
        size_t indexCount;
        size_t indexBufferSize;
        WGPUBuffer uniformBuffer;
        WGPUBindGroup bindGroup;
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

private:
    Renderable createSphereRenderable(float radius, int widthSegments = 32, int heightSegments = 16, float randomness = 0.0f);
    WGPUBindGroup createSphereBindGroup(WGPUBuffer uniformBuffer, WGPUTextureView textureView, const glm::mat4& transform);
    WGPUBuffer createSphereUniformBuffer(const glm::mat4& transform);
    void ensureEnoughAsteroids();

private:
    WGPUTexture m_depthTexture = nullptr;
    WGPUTexture m_moonImageTexture = nullptr;
    WGPUTextureView m_moonImageTextureView = nullptr;
    WGPUTexture m_planetImageTexture = nullptr;
    WGPUTextureView m_planetImageTextureView = nullptr;
    WGPUSampler m_sampler = nullptr;
    WGPUBuffer m_uniformBuffer = nullptr;
    WGPUBindGroup m_bindGroup = nullptr;
    WGPUBindGroupLayout m_bindGroupLayout = nullptr;
    WGPUBindGroupLayout m_sphereBindGroupLayout = nullptr;
    WGPUPipelineLayout m_pipelineLayout = nullptr;
    WGPURenderPipeline m_renderPipeline = nullptr;
    WGPUShaderModule m_vertWGSLShaderModule = nullptr;
    WGPUShaderModule m_fragWGSLShaderModule = nullptr;
    WGPURenderBundle m_renderBundle = nullptr;

    Renderable m_planet{};
    std::vector<Renderable> m_renderables{};
    std::vector<Renderable> m_asteroids{};

    glm::mat4 m_projectionMatrix{ glm::mat4(1.0) };
    glm::mat4 m_modelViewProjectionMatrix{ glm::mat4(1.0) };

    int m_asteroidCount = 1000;
    int m_currentAsteroidCount = 1000;
    bool m_useRenderBundles = true;
};

} // namespace jipu