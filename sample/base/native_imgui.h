
#pragma once

#include <chrono>
#include <glm/glm.hpp>
#include <imgui.h>

#include "jipu/native/adapter.h"
#include "jipu/native/buffer.h"
#include "jipu/native/command_buffer.h"
#include "jipu/native/command_encoder.h"
#include "jipu/native/device.h"
#include "jipu/native/physical_device.h"
#include "jipu/native/pipeline.h"
#include "jipu/native/pipeline_layout.h"
#include "jipu/native/queue.h"
#include "jipu/native/surface.h"
#include "jipu/native/swapchain.h"

namespace jipu
{

class NativeImGui
{
    friend class NativeSample;

public:
    void record(std::vector<std::function<void()>> cmds);
    void window(const char* title, std::vector<std::function<void()>> uis);

public:
    void init(Device* device, Queue* queue, Swapchain* swapchain);
    void clear();
    void build();
    void draw(CommandEncoder* commandEncoder, TextureView* renderView);

private:
    Device* m_device = nullptr;
    Queue* m_queue = nullptr;
    Swapchain* m_swapchain = nullptr;

    struct UITransform
    {
        glm::vec2 scale = { 0, 0 };
        glm::vec2 translate = { 0, 0 };
    } m_uiTransform;

    std::unique_ptr<Buffer> m_vertexBuffer = nullptr;
    std::unique_ptr<Buffer> m_indexBuffer = nullptr;
    std::unique_ptr<Buffer> m_uniformBuffer = nullptr;
    std::unique_ptr<Buffer> m_fontBuffer = nullptr;
    std::unique_ptr<Texture> m_fontTexture = nullptr;
    std::unique_ptr<TextureView> m_fontTextureView = nullptr;
    std::unique_ptr<Sampler> m_fontSampler = nullptr;
    std::vector<std::unique_ptr<BindGroupLayout>> m_bindGroupLayouts{};
    std::vector<std::unique_ptr<BindGroup>> m_bindGroups{};
    std::unique_ptr<PipelineLayout> m_pipelineLayout = nullptr;
    std::unique_ptr<RenderPipeline> m_pipeline = nullptr;
};

} // namespace jipu