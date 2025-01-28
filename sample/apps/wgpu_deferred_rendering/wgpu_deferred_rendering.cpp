#include "wgpu_deferred_rendering.h"

#include "file.h"
#include "image.h"

#include <chrono>
#include <cmath>
#include <random>
#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace jipu
{

static const int kMaxNumLights = 1024;
static const uint32_t lightDataStride = 8;
static glm::vec3 lightExtentMin{ -50.f, -30.f, -50.f };
static glm::vec3 lightExtentMax{ 50.f, 50.f, 50.f };

WGPUDeferredRenderingSample::WGPUDeferredRenderingSample(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
    m_imgui.emplace(this);
}

WGPUDeferredRenderingSample::~WGPUDeferredRenderingSample()
{
    finalizeContext();
}

void WGPUDeferredRenderingSample::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);

    float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    float fov = (2.0f * glm::pi<float>()) / 5.0f;
    float nearPlane = 1.0f;
    float farPlane = 2000.0f;

    m_projectionMatrix = glm::perspective(fov, aspect, nearPlane, farPlane);
}

void WGPUDeferredRenderingSample::onBeforeUpdate()
{
    WGPUSample::onBeforeUpdate();

    static const char* modes[] = { "rendering", "gBuffers View" };

    recordImGui({ [&]() {
        windowImGui(
            "Deferred Rendering", { [&]() {
                ImGui::Combo("mode", &m_mode, modes, IM_ARRAYSIZE(modes));
                ImGui::SliderInt("numLights", &m_numLights, 1, kMaxNumLights);
            } });
    } });
}

void WGPUDeferredRenderingSample::onUpdate()
{
    WGPUSample::onUpdate();

    static const glm::vec3 eyePosition{ 0.f, 50.f, -100.f };
    static const glm::vec3 upVector{ 0.f, 1.f, 0.f };
    static const glm::vec3 origin{ 0.f, 0.f, 0.f };

    auto getCameraViewProjMatrix = [&]() -> glm::mat4 {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        const double rad = static_cast<double>(M_PI) * (now / 5000.0);

        const glm::dmat4 rotation = glm::rotate(glm::dmat4(1.0), rad, glm::dvec3(0, 1, 0));
        const glm::dvec3 rotatedEyePosition = glm::dvec3(rotation * glm::dvec4(eyePosition, 1.0));
        const glm::dmat4 viewMatrix = glm::lookAt(rotatedEyePosition, glm::dvec3(origin), glm::dvec3(upVector));

        return m_projectionMatrix * glm::mat4(viewMatrix);
    };

    {
        const auto cameraViewProj = getCameraViewProjMatrix();
        wgpu.QueueWriteBuffer(m_queue, m_cameraUniformBuffer, 0, &cameraViewProj, sizeof(cameraViewProj));

        const auto cameraInvViewProj = glm::inverse(cameraViewProj);
        wgpu.QueueWriteBuffer(m_queue, m_cameraUniformBuffer, 64, &cameraInvViewProj, sizeof(cameraInvViewProj));
    }

    {
        wgpu.QueueWriteBuffer(m_queue, m_configUniformBuffer, 0, &m_numLights, sizeof(uint32_t));
    }
}

void WGPUDeferredRenderingSample::onDraw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu.SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu.TextureCreateView(surfaceTexture.texture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu.DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    {
        {
            std::array<WGPURenderPassColorAttachment, 2> colorAttachments{
                WGPURenderPassColorAttachment{
                    .view = m_float16TextureView,
                    .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                    .loadOp = WGPULoadOp_Clear,
                    .storeOp = WGPUStoreOp_Store,
                    .clearValue = { .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f },
                },
                WGPURenderPassColorAttachment{
                    .view = m_albedoTextureView,
                    .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                    .loadOp = WGPULoadOp_Clear,
                    .storeOp = WGPUStoreOp_Store,
                    .clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
                },
            };

            WGPURenderPassDepthStencilAttachment depthStencilAttachment{};
            depthStencilAttachment.view = m_depthTextureView;
            depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
            depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
            depthStencilAttachment.depthClearValue = 1.0f;

            WGPURenderPassDescriptor renderPassDescriptor{};
            renderPassDescriptor.colorAttachmentCount = colorAttachments.size();
            renderPassDescriptor.colorAttachments = colorAttachments.data();
            renderPassDescriptor.depthStencilAttachment = &depthStencilAttachment;

            WGPURenderPassEncoder renderPassEncoder = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
            wgpu.RenderPassEncoderSetPipeline(renderPassEncoder, m_gBufferWriteRenderPipeline);
            wgpu.RenderPassEncoderSetBindGroup(renderPassEncoder, 0, m_sceneUniformBindGroup, 0, nullptr);
            wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_vertexBuffer, 0, m_dragonMesh.positions.size() * 8 * sizeof(float));
            wgpu.RenderPassEncoderSetIndexBuffer(renderPassEncoder, m_indexBuffer, WGPUIndexFormat_Uint16, 0, m_dragonMesh.triangles.size() * 3 * sizeof(uint16_t));
            wgpu.RenderPassEncoderDrawIndexed(renderPassEncoder, static_cast<uint32_t>(m_dragonMesh.triangles.size() * 3), 1, 0, 0, 0);
            wgpu.RenderPassEncoderEnd(renderPassEncoder);
            wgpu.RenderPassEncoderRelease(renderPassEncoder);
        }
        {
            WGPUComputePassDescriptor computePassDescriptor{};
            WGPUComputePassEncoder computePassEncoder = wgpu.CommandEncoderBeginComputePass(commandEncoder, &computePassDescriptor);
            wgpu.ComputePassEncoderSetPipeline(computePassEncoder, m_lightComputePipeline);
            wgpu.ComputePassEncoderSetBindGroup(computePassEncoder, 0, m_lightBufferComputeBindGroup, 0, nullptr);
            wgpu.ComputePassEncoderDispatchWorkgroups(computePassEncoder, std::ceil(static_cast<float>(kMaxNumLights) / 64), 1, 1);
            wgpu.ComputePassEncoderEnd(computePassEncoder);
            wgpu.ComputePassEncoderRelease(computePassEncoder);
        }

        std::array<WGPURenderPassColorAttachment, 1> textureQuadColorAttachments{
            WGPURenderPassColorAttachment{
                .view = surfaceTextureView,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f },
            },
        };

        WGPURenderPassDescriptor textureQuadPassDescriptor{};
        textureQuadPassDescriptor.colorAttachmentCount = textureQuadColorAttachments.size();
        textureQuadPassDescriptor.colorAttachments = textureQuadColorAttachments.data();

        if (m_mode == 0)
        {
            WGPURenderPassEncoder textureQuadPass = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &textureQuadPassDescriptor);
            wgpu.RenderPassEncoderSetPipeline(textureQuadPass, m_deferredRenderingRenderPipeline);
            wgpu.RenderPassEncoderSetBindGroup(textureQuadPass, 0, m_gBufferTextureBindGroup, 0, nullptr);
            wgpu.RenderPassEncoderSetBindGroup(textureQuadPass, 1, m_lightBufferBindGroup, 0, nullptr);
            wgpu.RenderPassEncoderDraw(textureQuadPass, 6, 1, 0, 0);
            wgpu.RenderPassEncoderEnd(textureQuadPass);
            wgpu.RenderPassEncoderRelease(textureQuadPass);
        }
        else
        {
            WGPURenderPassEncoder textureQuadPass = wgpu.CommandEncoderBeginRenderPass(commandEncoder, &textureQuadPassDescriptor);
            wgpu.RenderPassEncoderSetPipeline(textureQuadPass, m_gBuffersDebugViewRenderPipeline);
            wgpu.RenderPassEncoderSetBindGroup(textureQuadPass, 0, m_gBufferTextureBindGroup, 0, nullptr);
            wgpu.RenderPassEncoderDraw(textureQuadPass, 6, 1, 0, 0);
            wgpu.RenderPassEncoderEnd(textureQuadPass);
            wgpu.RenderPassEncoderRelease(textureQuadPass);
        }
    }

    drawImGui(commandEncoder, surfaceTextureView);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu.QueueSubmit(m_queue, 1, &commandBuffer);
    wgpu.SurfacePresent(m_surface);

    wgpu.CommandBufferRelease(commandBuffer);
    wgpu.CommandEncoderRelease(commandEncoder);
    wgpu.TextureViewRelease(surfaceTextureView);
    wgpu.TextureRelease(surfaceTexture.texture);
}

void WGPUDeferredRenderingSample::initializeContext()
{
    WGPUSample::initializeContext();

    createVertexBuffer();
    createIndexBuffer();
    createModelUniformBuffer();
    createCameraUniformBuffer();
    createConfigUniformBuffer();
    createLightBuffer();
    createLightExtentBuffer();
    createFloat16Texture();
    createAlbedoTexture();
    createDepthTexture();
    createFloat16TextureView();
    createAlbedoTextureView();
    createDepthTextureView();

    createShaderModules();

    createSceneUniformBindGroupLayout();
    createSceneUniformBindGroup();
    createGBufferWritePipelineLayout();
    createGBufferWriteRenderPipeline();

    createLightBufferBindGroupLayout();
    createLightBufferBindGroup();
    createLightBufferComputeBindGroupLayout();
    createLightBufferComputeBindGroup();
    createLightPipelineLayout();
    createLightComputePipeline();

    createGBufferTextureBindGroupLayout();
    createGBufferTextureBindGroup();
    createDeferredRenderingPipelineLayout();
    createDeferredRenderingRenderPipeline();

    createGBuffersDebugViewPipelineLayout();
    createGBuffersDebugViewRenderPipeline();
}

void WGPUDeferredRenderingSample::finalizeContext()
{
    if (m_vertexBuffer)
    {
        wgpu.BufferRelease(m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }

    if (m_indexBuffer)
    {
        wgpu.BufferRelease(m_indexBuffer);
        m_indexBuffer = nullptr;
    }

    if (m_modelUniformBuffer)
    {
        wgpu.BufferRelease(m_modelUniformBuffer);
        m_modelUniformBuffer = nullptr;
    }

    if (m_cameraUniformBuffer)
    {
        wgpu.BufferRelease(m_cameraUniformBuffer);
        m_cameraUniformBuffer = nullptr;
    }

    if (m_configUniformBuffer)
    {
        wgpu.BufferRelease(m_configUniformBuffer);
        m_configUniformBuffer = nullptr;
    }

    if (m_lightBuffer)
    {
        wgpu.BufferRelease(m_lightBuffer);
        m_lightBuffer = nullptr;
    }

    if (m_lightExtentBuffer)
    {
        wgpu.BufferRelease(m_lightExtentBuffer);
        m_lightExtentBuffer = nullptr;
    }

    if (m_float16Texture)
    {
        wgpu.TextureRelease(m_float16Texture);
        m_float16Texture = nullptr;
    }

    if (m_albedoTexture)
    {
        wgpu.TextureRelease(m_albedoTexture);
        m_albedoTexture = nullptr;
    }

    if (m_depthTexture)
    {
        wgpu.TextureRelease(m_depthTexture);
        m_depthTexture = nullptr;
    }

    if (m_float16TextureView)
    {
        wgpu.TextureViewRelease(m_float16TextureView);
        m_float16TextureView = nullptr;
    }

    if (m_albedoTextureView)
    {
        wgpu.TextureViewRelease(m_albedoTextureView);
        m_albedoTextureView = nullptr;
    }

    if (m_depthTextureView)
    {
        wgpu.TextureViewRelease(m_depthTextureView);
        m_depthTextureView = nullptr;
    }

    if (m_fragmentDeferredRenderingShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragmentDeferredRenderingShaderModule);
        m_fragmentDeferredRenderingShaderModule = nullptr;
    }

    if (m_fragmentGBufferDebugViewShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragmentGBufferDebugViewShaderModule);
        m_fragmentGBufferDebugViewShaderModule = nullptr;
    }

    if (m_fragmentWriteGBuffersShaderModule)
    {
        wgpu.ShaderModuleRelease(m_fragmentWriteGBuffersShaderModule);
        m_fragmentWriteGBuffersShaderModule = nullptr;
    }

    if (m_vertexTextureQuadShaderModule)
    {
        wgpu.ShaderModuleRelease(m_vertexTextureQuadShaderModule);
        m_vertexTextureQuadShaderModule = nullptr;
    }

    if (m_vertexWriteGBuffersShaderModule)
    {
        wgpu.ShaderModuleRelease(m_vertexWriteGBuffersShaderModule);
        m_vertexWriteGBuffersShaderModule = nullptr;
    }

    if (m_lightUpdateShaderModule)
    {
        wgpu.ShaderModuleRelease(m_lightUpdateShaderModule);
        m_lightUpdateShaderModule = nullptr;
    }

    if (m_sceneUniformBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_sceneUniformBindGroupLayout);
        m_sceneUniformBindGroupLayout = nullptr;
    }

    if (m_sceneUniformBindGroup)
    {
        wgpu.BindGroupRelease(m_sceneUniformBindGroup);
        m_sceneUniformBindGroup = nullptr;
    }

    if (m_gBufferWritePipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_gBufferWritePipelineLayout);
        m_gBufferWritePipelineLayout = nullptr;
    }

    if (m_gBufferWriteRenderPipeline)
    {
        wgpu.RenderPipelineRelease(m_gBufferWriteRenderPipeline);
        m_gBufferWriteRenderPipeline = nullptr;
    }

    if (m_lightBufferBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_lightBufferBindGroupLayout);
        m_lightBufferBindGroupLayout = nullptr;
    }

    if (m_lightBufferBindGroup)
    {
        wgpu.BindGroupRelease(m_lightBufferBindGroup);
        m_lightBufferBindGroup = nullptr;
    }

    if (m_lightBufferComputeBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_lightBufferComputeBindGroupLayout);
        m_lightBufferComputeBindGroupLayout = nullptr;
    }

    if (m_lightBufferComputeBindGroup)
    {
        wgpu.BindGroupRelease(m_lightBufferComputeBindGroup);
        m_lightBufferComputeBindGroup = nullptr;
    }

    if (m_lightPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_lightPipelineLayout);
        m_lightPipelineLayout = nullptr;
    }

    if (m_lightComputePipeline)
    {
        wgpu.ComputePipelineRelease(m_lightComputePipeline);
        m_lightComputePipeline = nullptr;
    }

    if (m_gBufferTextureBindGroupLayout)
    {
        wgpu.BindGroupLayoutRelease(m_gBufferTextureBindGroupLayout);
        m_gBufferTextureBindGroupLayout = nullptr;
    }

    if (m_gBufferTextureBindGroup)
    {
        wgpu.BindGroupRelease(m_gBufferTextureBindGroup);
        m_gBufferTextureBindGroup = nullptr;
    }

    if (m_deferredRenderingPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_deferredRenderingPipelineLayout);
        m_deferredRenderingPipelineLayout = nullptr;
    }

    if (m_deferredRenderingRenderPipeline)
    {
        wgpu.RenderPipelineRelease(m_deferredRenderingRenderPipeline);
        m_deferredRenderingRenderPipeline = nullptr;
    }

    if (m_gBuffersDebugViewPipelineLayout)
    {
        wgpu.PipelineLayoutRelease(m_gBuffersDebugViewPipelineLayout);
        m_gBuffersDebugViewPipelineLayout = nullptr;
    }

    if (m_gBuffersDebugViewRenderPipeline)
    {
        wgpu.RenderPipelineRelease(m_gBuffersDebugViewRenderPipeline);
        m_gBuffersDebugViewRenderPipeline = nullptr;
    }

    WGPUSample::finalizeContext();
}

void WGPUDeferredRenderingSample::createVertexBuffer()
{
    // Create the model vertex buffer.
    const uint32_t vertexStride = 8;
    WGPUBufferDescriptor vertexBufferDescriptor{};
    vertexBufferDescriptor.size = m_dragonMesh.positions.size() * vertexStride * sizeof(float);
    vertexBufferDescriptor.usage = WGPUBufferUsage_Vertex;
    vertexBufferDescriptor.mappedAtCreation = true;

    m_vertexBuffer = wgpu.DeviceCreateBuffer(m_device, &vertexBufferDescriptor);
    assert(m_vertexBuffer);

    void* mappedVertexPtr = wgpu.BufferGetMappedRange(m_vertexBuffer, 0, vertexBufferDescriptor.size);
    auto vertexBuffer = reinterpret_cast<float*>(mappedVertexPtr);
    for (auto i = 0; i < m_dragonMesh.positions.size(); ++i)
    {
        memcpy(vertexBuffer + vertexStride * i, &m_dragonMesh.positions[i], 3 * sizeof(float));
        memcpy(vertexBuffer + vertexStride * i + 3, &m_dragonMesh.normals[i], 3 * sizeof(float));
        memcpy(vertexBuffer + vertexStride * i + 6, &m_dragonMesh.uvs[i], 2 * sizeof(float));
    }

    wgpu.BufferUnmap(m_vertexBuffer);
}

void WGPUDeferredRenderingSample::createIndexBuffer()
{
    // Create the model index buffer.
    const uint32_t indexCount = static_cast<uint32_t>(m_dragonMesh.triangles.size() * 3);
    WGPUBufferDescriptor indexBufferDescriptor{};
    indexBufferDescriptor.size = indexCount * sizeof(uint16_t);
    indexBufferDescriptor.usage = WGPUBufferUsage_Index;
    indexBufferDescriptor.mappedAtCreation = true;

    m_indexBuffer = wgpu.DeviceCreateBuffer(m_device, &indexBufferDescriptor);
    assert(m_indexBuffer);

    {
        void* mappedIndexPtr = wgpu.BufferGetMappedRange(m_indexBuffer, 0, indexBufferDescriptor.size);
        auto indexBuffer = reinterpret_cast<uint16_t*>(mappedIndexPtr);
        for (auto i = 0; i < m_dragonMesh.triangles.size(); ++i)
        {
            memcpy(indexBuffer + 3 * i, &m_dragonMesh.triangles[i], 3 * sizeof(uint16_t));
        }

        wgpu.BufferUnmap(m_indexBuffer);
    }
}

void WGPUDeferredRenderingSample::createModelUniformBuffer()
{
    // Create the model uniform buffer.
    WGPUBufferDescriptor modelUniformBufferDescriptor{};
    modelUniformBufferDescriptor.size = sizeof(ModelUniform);
    modelUniformBufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;

    m_modelUniformBuffer = wgpu.DeviceCreateBuffer(m_device, &modelUniformBufferDescriptor);
    assert(m_modelUniformBuffer);

    // Move the model so it's centered.
    // const modelMatrix = mat4.translation([ 0, -45, 0 ]);
    const glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, -45, 0));
    wgpu.QueueWriteBuffer(m_queue, m_modelUniformBuffer, 0, &modelMatrix, sizeof(modelMatrix));

    const glm::mat4 invertTransposeModelMatrix = glm::transpose(modelMatrix);
    wgpu.QueueWriteBuffer(m_queue, m_modelUniformBuffer, 64, &invertTransposeModelMatrix, sizeof(invertTransposeModelMatrix));
}

void WGPUDeferredRenderingSample::createCameraUniformBuffer()
{
    // Create the camera uniform buffer.
    WGPUBufferDescriptor cameraUniformBufferDescriptor{};
    cameraUniformBufferDescriptor.size = sizeof(CameraUniform);
    cameraUniformBufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;

    m_cameraUniformBuffer = wgpu.DeviceCreateBuffer(m_device, &cameraUniformBufferDescriptor);
    assert(m_cameraUniformBuffer);
}

void WGPUDeferredRenderingSample::createConfigUniformBuffer()
{
    // Create the config uniform buffer.
    WGPUBufferDescriptor configUniformBufferDescriptor{};
    configUniformBufferDescriptor.size = sizeof(uint32_t);
    configUniformBufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    configUniformBufferDescriptor.mappedAtCreation = true;

    m_configUniformBuffer = wgpu.DeviceCreateBuffer(m_device, &configUniformBufferDescriptor);
    assert(m_configUniformBuffer);

    uint32_t* configData = reinterpret_cast<uint32_t*>(wgpu.BufferGetMappedRange(m_configUniformBuffer, 0, sizeof(uint32_t)));
    configData[0] = m_numLights;

    wgpu.BufferUnmap(m_configUniformBuffer);
}

void WGPUDeferredRenderingSample::createLightBuffer()
{
    glm::vec3 extent = lightExtentMax - lightExtentMin;
    const size_t bufferSizeInByte = sizeof(float) * lightDataStride * kMaxNumLights;

    WGPUBufferDescriptor lightBufferDescriptor{};
    lightBufferDescriptor.size = bufferSizeInByte;
    lightBufferDescriptor.usage = WGPUBufferUsage_Storage;
    lightBufferDescriptor.mappedAtCreation = true;

    m_lightBuffer = wgpu.DeviceCreateBuffer(m_device, &lightBufferDescriptor);
    assert(m_lightBuffer);

    float* lightData = reinterpret_cast<float*>(wgpu.BufferGetMappedRange(m_lightBuffer, 0, bufferSizeInByte));

    // 3. 난수 생성기 및 분포 설정
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f); // 0~1 범위

    // 4. 광원 데이터 채우기
    for (int i = 0; i < kMaxNumLights; ++i)
    {
        int offset = i * lightDataStride;

        // (1) 위치
        float px = dist01(gen) * extent.x + lightExtentMin.x;
        float py = dist01(gen) * extent.y + lightExtentMin.y;
        float pz = dist01(gen) * extent.z + lightExtentMin.z;
        float pw = 1.0f;

        lightData[offset + 0] = px;
        lightData[offset + 1] = py;
        lightData[offset + 2] = pz;
        lightData[offset + 3] = pw;

        // (2) 색상 + 반지름
        // 색상은 (0~2) 범위
        float cx = dist01(gen) * 2.0f;
        float cy = dist01(gen) * 2.0f;
        float cz = dist01(gen) * 2.0f;
        float cw = 20.0f; // 반지름

        lightData[offset + 4] = cx;
        lightData[offset + 5] = cy;
        lightData[offset + 6] = cz;
        lightData[offset + 7] = cw;
    }

    wgpu.BufferUnmap(m_lightBuffer);
}

void WGPUDeferredRenderingSample::createLightExtentBuffer()
{
    // Create the light extent buffer.
    WGPUBufferDescriptor lightExtentBufferDescriptor{};
    lightExtentBufferDescriptor.size = 4 * 8;
    lightExtentBufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;

    m_lightExtentBuffer = wgpu.DeviceCreateBuffer(m_device, &lightExtentBufferDescriptor);
    assert(m_lightExtentBuffer);

    // float lightExtentData[8];
    // memcpy(lightExtentData, &m_lightExtentMin, 3 * sizeof(float));
    // memcpy(lightExtentData + 4, &m_lightExtentMax, 3 * sizeof(float));

    std::array<float, 8> lightExtentData{
        m_lightExtentMin.x, m_lightExtentMin.y, m_lightExtentMin.z, 0.0f,
        m_lightExtentMax.x, m_lightExtentMax.y, m_lightExtentMax.z, 0.0f
    };

    wgpu.QueueWriteBuffer(m_queue, m_lightExtentBuffer, 0, lightExtentData.data(), lightExtentData.size() * sizeof(float));
}

void WGPUDeferredRenderingSample::createFloat16Texture()
{
    // Create the float16 texture.
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = WGPUTextureDimension_2D;
    textureDescriptor.size.width = m_width;
    textureDescriptor.size.height = m_height;
    textureDescriptor.size.depthOrArrayLayers = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.format = WGPUTextureFormat_RGBA16Float;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;

    m_float16Texture = wgpu.DeviceCreateTexture(m_device, &textureDescriptor);
    assert(m_float16Texture);
}

void WGPUDeferredRenderingSample::createAlbedoTexture()
{
    // Create the albedo texture.
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = WGPUTextureDimension_2D;
    textureDescriptor.size.width = m_width;
    textureDescriptor.size.height = m_height;
    textureDescriptor.size.depthOrArrayLayers = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.format = WGPUTextureFormat_BGRA8Unorm;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;

    m_albedoTexture = wgpu.DeviceCreateTexture(m_device, &textureDescriptor);
    assert(m_albedoTexture);
}

void WGPUDeferredRenderingSample::createDepthTexture()
{
    // Create the depth texture.
    WGPUTextureDescriptor textureDescriptor{};
    textureDescriptor.dimension = WGPUTextureDimension_2D;
    textureDescriptor.size.width = m_width;
    textureDescriptor.size.height = m_height;
    textureDescriptor.size.depthOrArrayLayers = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.format = WGPUTextureFormat_Depth24Plus;
    textureDescriptor.mipLevelCount = 1;
    textureDescriptor.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;

    m_depthTexture = wgpu.DeviceCreateTexture(m_device, &textureDescriptor);
    assert(m_depthTexture);
}

void WGPUDeferredRenderingSample::createFloat16TextureView()
{
    WGPUTextureViewDescriptor textureViewDescriptor{};
    textureViewDescriptor.format = WGPUTextureFormat_RGBA16Float;
    textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    textureViewDescriptor.aspect = WGPUTextureAspect_All;
    textureViewDescriptor.baseMipLevel = 0;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;

    m_float16TextureView = wgpu.TextureCreateView(m_float16Texture, &textureViewDescriptor);
    assert(m_float16TextureView);
}

void WGPUDeferredRenderingSample::createAlbedoTextureView()
{
    WGPUTextureViewDescriptor textureViewDescriptor{};
    textureViewDescriptor.format = WGPUTextureFormat_BGRA8Unorm;
    textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    textureViewDescriptor.aspect = WGPUTextureAspect_All;
    textureViewDescriptor.baseMipLevel = 0;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;

    m_albedoTextureView = wgpu.TextureCreateView(m_albedoTexture, &textureViewDescriptor);
    assert(m_albedoTextureView);
}

void WGPUDeferredRenderingSample::createDepthTextureView()
{
    WGPUTextureViewDescriptor textureViewDescriptor{};
    textureViewDescriptor.format = WGPUTextureFormat_Depth24Plus;
    textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
    textureViewDescriptor.aspect = WGPUTextureAspect_All;
    textureViewDescriptor.baseMipLevel = 0;
    textureViewDescriptor.mipLevelCount = 1;
    textureViewDescriptor.baseArrayLayer = 0;
    textureViewDescriptor.arrayLayerCount = 1;

    m_depthTextureView = wgpu.TextureCreateView(m_depthTexture, &textureViewDescriptor);
    assert(m_depthTextureView);
}

void WGPUDeferredRenderingSample::createShaderModules()
{
    {
        std::vector<char> fragmentDeferredRenderingShaderSource = utils::readFile(m_appDir / "fragmentDeferredRendering.wgsl", m_handle);

        std::string fragmentDeferredRenderingShaderCode(fragmentDeferredRenderingShaderSource.begin(), fragmentDeferredRenderingShaderSource.end());

        WGPUShaderModuleWGSLDescriptor fragmentDeferredRenderingShaderModuleWGSLDescriptor{};
        fragmentDeferredRenderingShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragmentDeferredRenderingShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = fragmentDeferredRenderingShaderCode.data(),
            .length = fragmentDeferredRenderingShaderCode.size(),
        };

        WGPUShaderModuleDescriptor fragmentDeferredRenderingShaderModuleDescriptor{};
        fragmentDeferredRenderingShaderModuleDescriptor.nextInChain = &fragmentDeferredRenderingShaderModuleWGSLDescriptor.chain;

        m_fragmentDeferredRenderingShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragmentDeferredRenderingShaderModuleDescriptor);
        assert(m_fragmentDeferredRenderingShaderModule);
    }

    {
        std::vector<char> fragmentGBufferDebugViewShaderSource = utils::readFile(m_appDir / "fragmentGBuffersDebugView.wgsl", m_handle);

        std::string fragmentGBufferDebugViewShaderCode(fragmentGBufferDebugViewShaderSource.begin(), fragmentGBufferDebugViewShaderSource.end());

        WGPUShaderModuleWGSLDescriptor fragmentGBufferDebugViewShaderModuleWGSLDescriptor{};
        fragmentGBufferDebugViewShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragmentGBufferDebugViewShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = fragmentGBufferDebugViewShaderCode.data(),
            .length = fragmentGBufferDebugViewShaderCode.size(),
        };

        WGPUShaderModuleDescriptor fragmentGBufferDebugViewShaderModuleDescriptor{};
        fragmentGBufferDebugViewShaderModuleDescriptor.nextInChain = &fragmentGBufferDebugViewShaderModuleWGSLDescriptor.chain;

        m_fragmentGBufferDebugViewShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragmentGBufferDebugViewShaderModuleDescriptor);
        assert(m_fragmentGBufferDebugViewShaderModule);
    }

    {
        std::vector<char> fragmentWriteGBuffersShaderSource = utils::readFile(m_appDir / "fragmentWriteGBuffers.wgsl", m_handle);

        std::string fragmentWriteGBuffersShaderCode(fragmentWriteGBuffersShaderSource.begin(), fragmentWriteGBuffersShaderSource.end());

        WGPUShaderModuleWGSLDescriptor fragmentWriteGBuffersShaderModuleWGSLDescriptor{};
        fragmentWriteGBuffersShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragmentWriteGBuffersShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = fragmentWriteGBuffersShaderCode.data(),
            .length = fragmentWriteGBuffersShaderCode.size(),
        };

        WGPUShaderModuleDescriptor fragmentWriteGBuffersShaderModuleDescriptor{};
        fragmentWriteGBuffersShaderModuleDescriptor.nextInChain = &fragmentWriteGBuffersShaderModuleWGSLDescriptor.chain;

        m_fragmentWriteGBuffersShaderModule = wgpu.DeviceCreateShaderModule(m_device, &fragmentWriteGBuffersShaderModuleDescriptor);
        assert(m_fragmentWriteGBuffersShaderModule);
    }

    {
        std::vector<char> vertexTextureQuadShaderSource = utils::readFile(m_appDir / "vertexTextureQuad.wgsl", m_handle);

        std::string vertexTextureQuadShaderCode(vertexTextureQuadShaderSource.begin(), vertexTextureQuadShaderSource.end());

        WGPUShaderModuleWGSLDescriptor vertexTextureQuadShaderModuleWGSLDescriptor{};
        vertexTextureQuadShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        vertexTextureQuadShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = vertexTextureQuadShaderCode.data(),
            .length = vertexTextureQuadShaderCode.size(),
        };

        WGPUShaderModuleDescriptor vertexTextureQuadShaderModuleDescriptor{};
        vertexTextureQuadShaderModuleDescriptor.nextInChain = &vertexTextureQuadShaderModuleWGSLDescriptor.chain;

        m_vertexTextureQuadShaderModule = wgpu.DeviceCreateShaderModule(m_device, &vertexTextureQuadShaderModuleDescriptor);
        assert(m_vertexTextureQuadShaderModule);
    }

    {
        std::vector<char> vertexWriteGBuffersShaderSource = utils::readFile(m_appDir / "vertexWriteGBuffers.wgsl", m_handle);

        std::string vertexWriteGBuffersShaderCode(vertexWriteGBuffersShaderSource.begin(), vertexWriteGBuffersShaderSource.end());

        WGPUShaderModuleWGSLDescriptor vertexWriteGBuffersShaderModuleWGSLDescriptor{};
        vertexWriteGBuffersShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        vertexWriteGBuffersShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = vertexWriteGBuffersShaderCode.data(),
            .length = vertexWriteGBuffersShaderCode.size(),
        };

        WGPUShaderModuleDescriptor vertexWriteGBuffersShaderModuleDescriptor{};
        vertexWriteGBuffersShaderModuleDescriptor.nextInChain = &vertexWriteGBuffersShaderModuleWGSLDescriptor.chain;

        m_vertexWriteGBuffersShaderModule = wgpu.DeviceCreateShaderModule(m_device, &vertexWriteGBuffersShaderModuleDescriptor);
        assert(m_vertexWriteGBuffersShaderModule);
    }

    {
        std::vector<char> lightUpdateShaderSource = utils::readFile(m_appDir / "lightUpdate.wgsl", m_handle);
        std::string lightUpdateShaderCode(lightUpdateShaderSource.begin(), lightUpdateShaderSource.end());

        WGPUShaderModuleWGSLDescriptor lightUpdateShaderModuleWGSLDescriptor{};
        lightUpdateShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        lightUpdateShaderModuleWGSLDescriptor.code = WGPUStringView{
            .data = lightUpdateShaderCode.data(),
            .length = lightUpdateShaderCode.size(),
        };

        WGPUShaderModuleDescriptor lightUpdateShaderModuleDescriptor{};
        lightUpdateShaderModuleDescriptor.nextInChain = &lightUpdateShaderModuleWGSLDescriptor.chain;

        m_lightUpdateShaderModule = wgpu.DeviceCreateShaderModule(m_device, &lightUpdateShaderModuleDescriptor);
        assert(m_lightUpdateShaderModule);
    }
}

void WGPUDeferredRenderingSample::createSceneUniformBindGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 2> bindGroupLayoutEntries{
        WGPUBindGroupLayoutEntry{
            .binding = 0,
            .visibility = WGPUShaderStage_Vertex,
            .buffer = WGPUBufferBindingLayout{
                .type = WGPUBufferBindingType_Uniform,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 1,
            .visibility = WGPUShaderStage_Vertex,
            .buffer = WGPUBufferBindingLayout{
                .type = WGPUBufferBindingType_Uniform,
            },
        }
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_sceneUniformBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_sceneUniformBindGroupLayout);
}

void WGPUDeferredRenderingSample::createSceneUniformBindGroup()
{
    std::array<WGPUBindGroupEntry, 2> bindGroupEntries{
        WGPUBindGroupEntry{
            .binding = 0,
            .buffer = m_modelUniformBuffer,
            .offset = 0,
            .size = sizeof(ModelUniform),
        },
        WGPUBindGroupEntry{
            .binding = 1,
            .buffer = m_cameraUniformBuffer,
            .offset = 0,
            .size = sizeof(CameraUniform),
        }
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_sceneUniformBindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    m_sceneUniformBindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_sceneUniformBindGroup);
}

void WGPUDeferredRenderingSample::createGBufferWritePipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_sceneUniformBindGroupLayout;

    m_gBufferWritePipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_gBufferWritePipelineLayout);
}

void WGPUDeferredRenderingSample::createGBufferWriteRenderPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_Back;

    std::array<WGPUVertexAttribute, 3> vertexAttributes{
        WGPUVertexAttribute{
            .shaderLocation = 0,
            .offset = 0,
            .format = WGPUVertexFormat_Float32x3,
        },
        WGPUVertexAttribute{
            .shaderLocation = 1,
            .offset = sizeof(float) * 3,
            .format = WGPUVertexFormat_Float32x3,
        },
        WGPUVertexAttribute{
            .shaderLocation = 2,
            .offset = sizeof(float) * 6,
            .format = WGPUVertexFormat_Float32x2,
        },
    };
    std::array<WGPUVertexBufferLayout, 1> vertexBufferLayout{
        WGPUVertexBufferLayout{
            .arrayStride = sizeof(float) * 8,
            .attributeCount = vertexAttributes.size(),
            .attributes = vertexAttributes.data(),
            .stepMode = WGPUVertexStepMode_Vertex,
        },
    };

    std::string vertexEntryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.bufferCount = vertexBufferLayout.size();
    vertexState.buffers = vertexBufferLayout.data();
    vertexState.module = m_vertexWriteGBuffersShaderModule;
    vertexState.entryPoint = WGPUStringView{ .data = vertexEntryPoint.data(), .length = vertexEntryPoint.size() };

    std::array<WGPUColorTargetState, 2> colorTargetStates{
        WGPUColorTargetState{
            .format = WGPUTextureFormat_RGBA16Float,
            .writeMask = WGPUColorWriteMask_All,
        },
        WGPUColorTargetState{
            .format = WGPUTextureFormat_BGRA8Unorm,
            .writeMask = WGPUColorWriteMask_All,
        },
    };

    std::string fragmentEntryPoint = "main";
    WGPUFragmentState fragmentState{};
    fragmentState.targetCount = colorTargetStates.size();
    fragmentState.targets = colorTargetStates.data();
    fragmentState.module = m_fragmentWriteGBuffersShaderModule;
    fragmentState.entryPoint = WGPUStringView{ .data = fragmentEntryPoint.data(), .length = fragmentEntryPoint.size() };

    WGPUDepthStencilState depthStencilState{};
    depthStencilState.format = WGPUTextureFormat_Depth24Plus;
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencilState.depthCompare = WGPUCompareFunction_Less;

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_gBufferWritePipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragmentState;
    renderPipelineDescriptor.depthStencil = &depthStencilState;
    renderPipelineDescriptor.multisample = multisampleState;

    m_gBufferWriteRenderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);
    assert(m_gBufferWriteRenderPipeline);
}

void WGPUDeferredRenderingSample::createLightBufferBindGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 3> bindGroupLayoutEntries{
        WGPUBindGroupLayoutEntry{
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .buffer = WGPUBufferBindingLayout{
                .type = WGPUBufferBindingType_ReadOnlyStorage,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 1,
            .visibility = WGPUShaderStage_Fragment,
            .buffer = WGPUBufferBindingLayout{
                .type = WGPUBufferBindingType_Uniform,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 2,
            .visibility = WGPUShaderStage_Fragment,
            .buffer = WGPUBufferBindingLayout{
                .type = WGPUBufferBindingType_Uniform,
            },
        }
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_lightBufferBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_lightBufferBindGroupLayout);
}

void WGPUDeferredRenderingSample::createLightBufferBindGroup()
{
    std::array<WGPUBindGroupEntry, 3> bindGroupEntries{
        WGPUBindGroupEntry{
            .binding = 0,
            .buffer = m_lightBuffer,
            .offset = 0,
            .size = sizeof(float) * lightDataStride * kMaxNumLights,
        },
        WGPUBindGroupEntry{
            .binding = 1,
            .buffer = m_configUniformBuffer,
            .offset = 0,
            .size = sizeof(uint32_t),
        },
        WGPUBindGroupEntry{
            .binding = 2,
            .buffer = m_cameraUniformBuffer,
            .offset = 0,
            .size = sizeof(CameraUniform),
        }
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_lightBufferBindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    m_lightBufferBindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_lightBufferBindGroup);
}

void WGPUDeferredRenderingSample::createLightBufferComputeBindGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 3> bindGroupLayoutEntries{
        WGPUBindGroupLayoutEntry{
            .binding = 0,
            .visibility = WGPUShaderStage_Compute,
            .buffer = {
                .type = WGPUBufferBindingType_Storage,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 1,
            .visibility = WGPUShaderStage_Compute,
            .buffer = {
                .type = WGPUBufferBindingType_Uniform,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 2,
            .visibility = WGPUShaderStage_Compute,
            .buffer = {
                .type = WGPUBufferBindingType_Uniform,
            },
        }
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_lightBufferComputeBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_lightBufferComputeBindGroupLayout);
}

void WGPUDeferredRenderingSample::createLightBufferComputeBindGroup()
{
    std::array<WGPUBindGroupEntry, 3> bindGroupEntries{
        WGPUBindGroupEntry{
            .binding = 0,
            .buffer = m_lightBuffer,
            .offset = 0,
            .size = sizeof(float) * lightDataStride * kMaxNumLights,
        },
        WGPUBindGroupEntry{
            .binding = 1,
            .buffer = m_configUniformBuffer,
            .offset = 0,
            .size = sizeof(uint32_t),
        },
        WGPUBindGroupEntry{
            .binding = 2,
            .buffer = m_lightExtentBuffer,
            .offset = 0,
            .size = 4 * 8,
        }
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_lightBufferComputeBindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    m_lightBufferComputeBindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_lightBufferComputeBindGroup);
}

void WGPUDeferredRenderingSample::createLightPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_lightBufferComputeBindGroupLayout;

    m_lightPipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_lightPipelineLayout);
}

void WGPUDeferredRenderingSample::createLightComputePipeline()
{
    std::string entryPoint = "main";
    WGPUComputePipelineDescriptor computePipelineDescriptor{};
    computePipelineDescriptor.layout = m_lightPipelineLayout;
    computePipelineDescriptor.compute.module = m_lightUpdateShaderModule;
    computePipelineDescriptor.compute.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };

    m_lightComputePipeline = wgpu.DeviceCreateComputePipeline(m_device, &computePipelineDescriptor);
    assert(m_lightComputePipeline);
}

void WGPUDeferredRenderingSample::createGBufferTextureBindGroupLayout()
{
    std::array<WGPUBindGroupLayoutEntry, 3> bindGroupLayoutEntries{
        WGPUBindGroupLayoutEntry{
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 1,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false,
            },
        },
        WGPUBindGroupLayoutEntry{
            .binding = 2,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_Depth,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = false,
            },
        }
    };

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
    bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    m_gBufferTextureBindGroupLayout = wgpu.DeviceCreateBindGroupLayout(m_device, &bindGroupLayoutDescriptor);
    assert(m_gBufferTextureBindGroupLayout);
}

void WGPUDeferredRenderingSample::createGBufferTextureBindGroup()
{

    std::array<WGPUBindGroupEntry, 3> bindGroupEntries{
        WGPUBindGroupEntry{
            .binding = 0,
            .textureView = m_float16TextureView,
        },
        WGPUBindGroupEntry{
            .binding = 1,
            .textureView = m_albedoTextureView,
        },
        WGPUBindGroupEntry{
            .binding = 2,
            .textureView = m_depthTextureView,
        }
    };

    WGPUBindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.layout = m_gBufferTextureBindGroupLayout;
    bindGroupDescriptor.entryCount = bindGroupEntries.size();
    bindGroupDescriptor.entries = bindGroupEntries.data();

    m_gBufferTextureBindGroup = wgpu.DeviceCreateBindGroup(m_device, &bindGroupDescriptor);
    assert(m_gBufferTextureBindGroup);
}

void WGPUDeferredRenderingSample::createDeferredRenderingPipelineLayout()
{
    std::array<WGPUBindGroupLayout, 2> bindGroupLayouts{
        m_gBufferTextureBindGroupLayout,
        m_lightBufferBindGroupLayout,
    };

    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = bindGroupLayouts.size();
    pipelineLayoutDescriptor.bindGroupLayouts = bindGroupLayouts.data();

    m_deferredRenderingPipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_deferredRenderingPipelineLayout);
}

void WGPUDeferredRenderingSample::createDeferredRenderingRenderPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_Back;

    std::string vertexEntryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.bufferCount = 0;
    vertexState.buffers = nullptr;
    vertexState.module = m_vertexTextureQuadShaderModule;
    vertexState.entryPoint = WGPUStringView{ .data = vertexEntryPoint.data(), .length = vertexEntryPoint.size() };

    std::array<WGPUColorTargetState, 1> colorTargetStates{
        WGPUColorTargetState{
            .format = m_surfaceConfigure.format,
            .writeMask = WGPUColorWriteMask_All,
        },
    };

    std::string fragmentEntryPoint = "main";
    WGPUFragmentState fragmentState{};
    fragmentState.targetCount = colorTargetStates.size();
    fragmentState.targets = colorTargetStates.data();
    fragmentState.module = m_fragmentDeferredRenderingShaderModule;
    fragmentState.entryPoint = WGPUStringView{ .data = fragmentEntryPoint.data(), .length = fragmentEntryPoint.size() };

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_deferredRenderingPipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragmentState;
    renderPipelineDescriptor.multisample = multisampleState;

    m_deferredRenderingRenderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);
    assert(m_deferredRenderingRenderPipeline);
}

void WGPUDeferredRenderingSample::createGBuffersDebugViewPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = &m_gBufferTextureBindGroupLayout;

    m_gBuffersDebugViewPipelineLayout = wgpu.DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
    assert(m_gBuffersDebugViewPipelineLayout);
}

void WGPUDeferredRenderingSample::createGBuffersDebugViewRenderPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_Back;

    std::string vertexEntryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.bufferCount = 0;
    vertexState.buffers = nullptr;
    vertexState.module = m_vertexTextureQuadShaderModule;
    vertexState.entryPoint = WGPUStringView{ .data = vertexEntryPoint.data(), .length = vertexEntryPoint.size() };

    std::array<WGPUColorTargetState, 1> colorTargetStates{
        WGPUColorTargetState{
            .format = m_surfaceConfigure.format,
            .writeMask = WGPUColorWriteMask_All,
        },
    };

    std::string key1 = "canvasSizeWidth";
    std::string key2 = "canvasSizeHeight";
    std::array<WGPUConstantEntry, 2> constantEntries{
        WGPUConstantEntry{
            .key = WGPUStringView{ .data = key1.data(), .length = key1.size() },
            .value = static_cast<double>(m_width),
        },
        WGPUConstantEntry{
            .key = WGPUStringView{ .data = key2.data(), .length = key2.size() },
            .value = static_cast<double>(m_height),
        }
    };

    std::string fragmentEntryPoint = "main";
    WGPUFragmentState fragmentState{};
    fragmentState.targetCount = colorTargetStates.size();
    fragmentState.targets = colorTargetStates.data();
    fragmentState.constantCount = constantEntries.size();
    fragmentState.constants = constantEntries.data();
    fragmentState.module = m_fragmentGBufferDebugViewShaderModule;
    fragmentState.entryPoint = WGPUStringView{ .data = fragmentEntryPoint.data(), .length = fragmentEntryPoint.size() };

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_gBuffersDebugViewPipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragmentState;
    renderPipelineDescriptor.multisample = multisampleState;

    m_gBuffersDebugViewRenderPipeline = wgpu.DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);
    assert(m_gBuffersDebugViewRenderPipeline);
}

} // namespace jipu
