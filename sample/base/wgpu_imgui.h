
#pragma once

#include "jipu/webgpu/webgpu_header.h"
#include <chrono>
#include <deque>
#include <functional>
#include <imgui.h>

namespace jipu
{

struct ImGui_ImplWGPU_InitInfo
{
    WGPUDevice device;
    int numFramesInFlight = 3;
    WGPUTextureFormat renderTargetFormat = WGPUTextureFormat_Undefined;
    WGPUTextureFormat depthStencilFormat = WGPUTextureFormat_Undefined;
    WGPUMultisampleState pipelineMultisampleState = {};

    ImGui_ImplWGPU_InitInfo()
    {
        pipelineMultisampleState.count = 1;
        pipelineMultisampleState.mask = UINT32_MAX;
        pipelineMultisampleState.alphaToCoverageEnabled = false;
    }
};

struct ImGui_ImplWGPU_RenderState
{
    WGPUDevice Device;
    WGPURenderPassEncoder RenderPassEncoder;
};

struct RenderResources
{
    WGPUTexture FontTexture = nullptr;                  // Font texture
    WGPUTextureView FontTextureView = nullptr;          // Texture view for font texture
    WGPUSampler Sampler = nullptr;                      // Sampler for the font texture
    WGPUBuffer Uniforms = nullptr;                      // Shader uniforms
    WGPUBindGroup CommonBindGroup = nullptr;            // Resources bind-group to bind the common resources to pipeline
    ImGuiStorage ImageBindGroups;                       // Resources bind-group to bind the font/image resources to pipeline (this is a key->value map)
    WGPUBindGroup ImageBindGroup = nullptr;             // Default font-resource of Dear ImGui
    WGPUBindGroupLayout ImageBindGroupLayout = nullptr; // Cache layout used for the image bind group. Avoids allocating unnecessary JS objects when working with WebASM
};

struct FrameResources
{
    WGPUBuffer IndexBuffer;
    WGPUBuffer VertexBuffer;
    ImDrawIdx* IndexBufferHost;
    ImDrawVert* VertexBufferHost;
    int IndexBufferSize;
    int VertexBufferSize;
};

struct Uniforms
{
    float MVP[4][4];
    float Gamma;
};

struct ImGui_ImplWGPU_Data
{
    ImGui_ImplWGPU_InitInfo initInfo;
    WGPUDevice wgpuDevice = nullptr;
    WGPUQueue defaultQueue = nullptr;
    WGPUTextureFormat renderTargetFormat = WGPUTextureFormat_Undefined;
    WGPUTextureFormat depthStencilFormat = WGPUTextureFormat_Undefined;
    WGPURenderPipeline pipelineState = nullptr;

    RenderResources renderResources;
    FrameResources* pFrameResources = nullptr;
    unsigned int numFramesInFlight = 0;
    unsigned int frameIndex = UINT_MAX;
};

class WGPUSample;
class WGPUImGui
{
public:
    explicit WGPUImGui(WGPUSample* sample);
    ~WGPUImGui();

public:
    void record(std::vector<std::function<void()>> cmds);
    void window(const char* title, std::vector<std::function<void()>> uis);

public:
    void initialize();
    void finalize();
    void build();
    void resize();
    void draw(WGPUCommandEncoder commandEncoder, WGPUTextureView renderView);

private:
    void SafeRelease(ImDrawIdx*& res);
    void SafeRelease(ImDrawVert*& res);
    void SafeRelease(WGPUBindGroupLayout& res);
    void SafeRelease(WGPUBindGroup& res);
    void SafeRelease(WGPUBuffer& res);
    void SafeRelease(WGPUPipelineLayout& res);
    void SafeRelease(WGPURenderPipeline& res);
    void SafeRelease(WGPUSampler& res);
    void SafeRelease(WGPUShaderModule& res);
    void SafeRelease(WGPUTextureView& res);
    void SafeRelease(WGPUTexture& res);
    void SafeRelease(RenderResources& res);
    void SafeRelease(FrameResources& res);

    ImGui_ImplWGPU_Data* getBackendData();
    WGPUProgrammableStageDescriptor createShaderModule(const char* wgsl_source);
    WGPUBindGroup createImageBindGroup(WGPUBindGroupLayout layout, WGPUTextureView texture);
    void createFontsTexture();
    void createUniformBuffer();
    bool createDeviceObjects();
    void invalidateDeviceObjects();
    void setupRenderState(ImDrawData* draw_data, WGPURenderPassEncoder ctx, FrameResources* fr);
    void renderDrawData(ImDrawData* draw_data, WGPURenderPassEncoder pass_encoder);
    bool init(ImGui_ImplWGPU_InitInfo* init_info);
    void shutdown();
    void newFrame();
    void drawPolyline(std::string title, std::deque<float> data, std::string unit);

private:
    WGPUSample* m_sample = nullptr;

    std::vector<std::function<void()>> m_cmds;
};

} // namespace jipu