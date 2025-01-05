
#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <imgui.h>

#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WGPUSample;
class WGPUImGui
{
public:
    explicit WGPUImGui(WGPUSample* sample);

public:
    void record(std::vector<std::function<void()>> cmds);
    void window(const char* title, std::vector<std::function<void()>> uis);

public:
    void init();
    void clear();
    void build();
    void draw(WGPUCommandEncoder commandEncoder, WGPUTextureView renderView);

protected:
    struct Padding
    {
        float top = 0.0f;
        float bottom = 0.0f;
        float left = 0.0f;
        float right = 0.0f;
    } m_padding;

private:
    void drawPolyline(std::string title, std::deque<float> data, std::string unit);

private:
    WGPUSample* m_sample = nullptr;

    struct Uniform
    {
        glm::mat4 mvp;
        float gamma;
    } m_uniform;

    WGPUBuffer m_vertexBuffer = nullptr;
    WGPUBuffer m_indexBuffer = nullptr;
    WGPUBuffer m_uniformBuffer = nullptr;
    WGPUBuffer m_fontBuffer = nullptr;
    WGPUTexture m_fontTexture = nullptr;
    WGPUTextureView m_fontTextureView = nullptr;
    WGPUSampler m_fontSampler = nullptr;
    std::vector<WGPUBindGroupLayout> m_bindGroupLayouts{};
    std::vector<WGPUBindGroup> m_bindGroups{};
    WGPUPipelineLayout m_pipelineLayout = nullptr;
    WGPURenderPipeline m_pipeline = nullptr;
};

} // namespace jipu