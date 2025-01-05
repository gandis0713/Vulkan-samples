#include "wgpu_im_gui.h"

#include "wgpu_sample.h"
#include <fmt/format.h>
#include <stdexcept>

#define MEMALIGN(_SIZE, _ALIGN) (((_SIZE) + ((_ALIGN) - 1)) & ~((_ALIGN) - 1)) // Memory align (copied from IM_ALIGN() macro).

namespace jipu
{

static const std::string __shader_vert_wgsl = R"(
struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec4<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) uv: vec2<f32>,
};

struct Uniforms {
    mvp: mat4x4<f32>,
    gamma: f32,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = uniforms.mvp * vec4<f32>(in.position, 0.0, 1.0);
    out.color = in.color;
    out.uv = in.uv;
    return out;
}
)";

static const std::string __shader_frag_wgsl = R"(
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) uv: vec2<f32>,
};

struct Uniforms {
    mvp: mat4x4<f32>,
    gamma: f32,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var s: sampler;
@group(1) @binding(0) var t: texture_2d<f32>;

@fragment
fn main(in: VertexOutput) -> @location(0) vec4<f32> {
    let color = in.color * textureSample(t, s, in.uv);
    let corrected_color = pow(color.rgb, vec3<f32>(uniforms.gamma));
    return vec4<f32>(corrected_color, color.a);
}
)";

WGPUImGui::WGPUImGui(WGPUSample* sample)
    : m_sample(sample)
{
}

void WGPUImGui::window(const char* title, std::vector<std::function<void()>> uis)
{
    // set windows position and size
    {
        auto scale = ImGui::GetIO().FontGlobalScale;
        ImGui::SetNextWindowPos(ImVec2(20, 200 + m_padding.top), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300 * scale, 100 * scale), ImGuiCond_FirstUseEver);
    }

    ImGui::Begin(title);
    for (auto& ui : uis)
        ui();
    ImGui::End();
}

void WGPUImGui::record(std::vector<std::function<void()>> cmds)
{
    ImGui::NewFrame();
    for (auto& cmd : cmds)
        cmd();
    ImGui::Render();
}

void WGPUImGui::init()
{
#if defined(__ANDROID__)
    m_padding.top = 80.0f;
    m_padding.bottom = 170.0f;
#endif

    // IMGUI_CHECKVERSION();
    ImGuiContext* imguiContext = ImGui::CreateContext();
    if (imguiContext == nullptr)
    {
        throw std::runtime_error("Failed to create imgui context");
    }

    ImGui::GetStyle().TouchExtraPadding = ImVec2(0.0f, 0.0f);

    ImGuiIO& io = ImGui::GetIO();
#if defined(__ANDROID__)
    io.FontGlobalScale = 3.0;
#else
    io.FontGlobalScale = 1.0;
#endif
    io.DisplaySize = ImVec2(m_sample->m_surfaceConfigure.width, m_sample->m_surfaceConfigure.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    // view background
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.5f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.5f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.5f);

    // Get texture for fonts.
    using FontDataType = unsigned char;
    FontDataType* fontData = nullptr;
    int fontTexWidth, fontTexHeight;
    io.Fonts->GetTexDataAsRGBA32(&fontData, &fontTexWidth, &fontTexHeight);

    // create font texture.
    {
        WGPUTextureDescriptor fontTextureDescriptor{};
        fontTextureDescriptor.dimension = WGPUTextureDimension_2D;
        fontTextureDescriptor.size.width = fontTexWidth;
        fontTextureDescriptor.size.height = fontTexHeight;
        fontTextureDescriptor.size.depthOrArrayLayers = 1;
        fontTextureDescriptor.sampleCount = 1;
        fontTextureDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
        fontTextureDescriptor.mipLevelCount = 1;
        fontTextureDescriptor.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;

        m_fontTexture = m_sample->wgpu.DeviceCreateTexture(m_sample->m_device, &fontTextureDescriptor);
    }

    // create font texture view.
    {
        WGPUTextureViewDescriptor fontTextureViewDescriptor{};
        fontTextureViewDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
        fontTextureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
        fontTextureViewDescriptor.aspect = WGPUTextureAspect_All;
        fontTextureViewDescriptor.baseMipLevel = 0;
        fontTextureViewDescriptor.mipLevelCount = 1;
        fontTextureViewDescriptor.baseArrayLayer = 0;
        fontTextureViewDescriptor.arrayLayerCount = 1;

        m_fontTextureView = m_sample->wgpu.TextureCreateView(m_fontTexture, &fontTextureViewDescriptor);
    }

    // create font staging buffer.
    {
        WGPUBufferDescriptor fontBufferDescriptor{};
        fontBufferDescriptor.size = fontTexWidth * fontTexHeight * 4 * sizeof(FontDataType);
        fontBufferDescriptor.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
        // fontBufferDescriptor.mappedAtCreation = true;

        m_fontBuffer = m_sample->wgpu.DeviceCreateBuffer(m_sample->m_device, &fontBufferDescriptor);

        // void* fontGPUMappedPointer = m_sample->wgpu.BufferGetMappedRange(m_fontBuffer, 0, fontBufferDescriptor.size);
        // memcpy(fontGPUMappedPointer, fontData, fontBufferDescriptor.size);
        // m_sample->wgpu.BufferUnmap(m_fontBuffer);

        m_sample->wgpu.QueueWriteBuffer(m_sample->m_queue, m_fontBuffer, 0, fontData, fontBufferDescriptor.size);
    }

    // copy buffer to texture
    {
        uint32_t channel = 4;
        uint32_t bytesPerData = sizeof(FontDataType);
        WGPUImageCopyBuffer copyTextureBuffer{};
        copyTextureBuffer.buffer = m_fontBuffer;
        copyTextureBuffer.layout = WGPUTextureDataLayout{
            .offset = 0,
            .bytesPerRow = static_cast<uint32_t>(fontTexWidth * 4 * sizeof(FontDataType)),
            .rowsPerImage = static_cast<uint32_t>(fontTexHeight),
        };

        WGPUImageCopyTexture copyTexture{};
        copyTexture.texture = m_fontTexture;
        copyTexture.aspect = WGPUTextureAspect_All;
        copyTexture.mipLevel = 0;
        copyTexture.origin = { 0, 0, 0 };

        WGPUExtent3D extent{};
        extent.width = fontTexWidth;
        extent.height = fontTexHeight;
        extent.depthOrArrayLayers = 1;

        WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
        auto commandEncoder = m_sample->wgpu.DeviceCreateCommandEncoder(m_sample->m_device, &commandEncoderDescriptor);
        m_sample->wgpu.CommandEncoderCopyBufferToTexture(commandEncoder, &copyTextureBuffer, &copyTexture, &extent);

        WGPUCommandBufferDescriptor commandBufferDescriptor{};
        auto commandBuffer = m_sample->wgpu.CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);
        m_sample->wgpu.QueueSubmit(m_sample->m_queue, 1, &commandBuffer);

        m_sample->wgpu.CommandBufferRelease(commandBuffer);
        m_sample->wgpu.CommandEncoderRelease(commandEncoder);
    }

    // create uniform buffer
    {
        WGPUBufferDescriptor uniformBufferDescriptor{};
        uniformBufferDescriptor.size = MEMALIGN(sizeof(Uniform), 16);
        uniformBufferDescriptor.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;

        m_uniformBuffer = m_sample->wgpu.DeviceCreateBuffer(m_sample->m_device, &uniformBufferDescriptor);
    }

    // create font sampler
    {
        WGPUSamplerDescriptor fontSamplerDescriptor{};
        fontSamplerDescriptor.addressModeU = WGPUAddressMode_ClampToEdge;
        fontSamplerDescriptor.addressModeV = WGPUAddressMode_ClampToEdge;
        fontSamplerDescriptor.addressModeW = WGPUAddressMode_ClampToEdge;
        fontSamplerDescriptor.lodMinClamp = 0.0f;
        fontSamplerDescriptor.lodMaxClamp = 32.0f;
        fontSamplerDescriptor.minFilter = WGPUFilterMode_Linear;
        fontSamplerDescriptor.magFilter = WGPUFilterMode_Linear;
        fontSamplerDescriptor.mipmapFilter = WGPUMipmapFilterMode_Linear;
        fontSamplerDescriptor.compare = WGPUCompareFunction_Undefined;
        fontSamplerDescriptor.maxAnisotropy = 1;

        m_fontSampler = m_sample->wgpu.DeviceCreateSampler(m_sample->m_device, &fontSamplerDescriptor);
    }

    // create binding group layout
    {
        m_bindGroupLayouts.resize(2);
        {
            WGPUBindGroupLayoutEntry uniformBindingLayoutEntry{};
            uniformBindingLayoutEntry.binding = 0;
            uniformBindingLayoutEntry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
            uniformBindingLayoutEntry.buffer = WGPUBufferBindingLayout{ .type = WGPUBufferBindingType_Uniform };

            WGPUBindGroupLayoutEntry fontSamplerBindingLayout{};
            fontSamplerBindingLayout.binding = 1;
            fontSamplerBindingLayout.visibility = WGPUShaderStage_Fragment;
            fontSamplerBindingLayout.sampler = WGPUSamplerBindingLayout{ .type = WGPUSamplerBindingType_Filtering };

            std::array<WGPUBindGroupLayoutEntry, 2> bindGroupLayoutEntries = { uniformBindingLayoutEntry, fontSamplerBindingLayout };

            WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
            bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();
            bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();

            m_bindGroupLayouts[0] = m_sample->wgpu.DeviceCreateBindGroupLayout(m_sample->m_device, &bindGroupLayoutDescriptor);
        }
        {
            WGPUBindGroupLayoutEntry fontTextureBindingLayout{};
            fontTextureBindingLayout.binding = 0;
            fontTextureBindingLayout.visibility = WGPUShaderStage_Fragment;
            fontTextureBindingLayout.texture = WGPUTextureBindingLayout{ .sampleType = WGPUTextureSampleType_Float,
                                                                         .viewDimension = WGPUTextureViewDimension_2D,
                                                                         .multisampled = WGPUOptionalBool_False };

            std::array<WGPUBindGroupLayoutEntry, 1> bindGroupLayoutEntries = { fontTextureBindingLayout };

            WGPUBindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
            bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();
            bindGroupLayoutDescriptor.entryCount = bindGroupLayoutEntries.size();

            m_bindGroupLayouts[1] = m_sample->wgpu.DeviceCreateBindGroupLayout(m_sample->m_device, &bindGroupLayoutDescriptor);
        }
    }

    // create binding group
    {
        m_bindGroups.resize(2);
        {
            WGPUBindGroupEntry uniformBinding{};
            uniformBinding.binding = 0;
            uniformBinding.buffer = m_uniformBuffer;
            uniformBinding.offset = 0;
            uniformBinding.size = MEMALIGN(sizeof(Uniform), 16);

            WGPUBindGroupEntry fontSamplerBinding{};
            fontSamplerBinding.binding = 1;
            fontSamplerBinding.sampler = m_fontSampler;

            std::array<WGPUBindGroupEntry, 2> bindGroupEntries = { uniformBinding, fontSamplerBinding };

            WGPUBindGroupDescriptor bindGroupDescriptor{};
            bindGroupDescriptor.layout = m_bindGroupLayouts[0];
            bindGroupDescriptor.entryCount = bindGroupEntries.size();
            bindGroupDescriptor.entries = bindGroupEntries.data();

            m_bindGroups[0] = m_sample->wgpu.DeviceCreateBindGroup(m_sample->m_device, &bindGroupDescriptor);
        }

        {

            WGPUBindGroupEntry fontTextureBinding{};
            fontTextureBinding.binding = 0;
            fontTextureBinding.textureView = m_fontTextureView;

            std::array<WGPUBindGroupEntry, 1> bindGroupEntries = { fontTextureBinding };

            WGPUBindGroupDescriptor bindGroupDescriptor{};
            bindGroupDescriptor.layout = m_bindGroupLayouts[1];
            bindGroupDescriptor.entryCount = bindGroupEntries.size();
            bindGroupDescriptor.entries = bindGroupEntries.data();

            m_bindGroups[1] = m_sample->wgpu.DeviceCreateBindGroup(m_sample->m_device, &bindGroupDescriptor);
        }
    }

    // create pipeline layout
    {
        WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.bindGroupLayoutCount = m_bindGroupLayouts.size();
        pipelineLayoutDescriptor.bindGroupLayouts = m_bindGroupLayouts.data();

        m_pipelineLayout = m_sample->wgpu.DeviceCreatePipelineLayout(m_sample->m_device, &pipelineLayoutDescriptor);
    }

    WGPUShaderModule vertWGSLShaderModule = nullptr;
    WGPUShaderModule fragWGSLShaderModule = nullptr;
    {
        WGPUShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
        vertexShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        vertexShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = __shader_vert_wgsl.data(), .length = __shader_vert_wgsl.size() };

        WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
        vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleWGSLDescriptor.chain;

        vertWGSLShaderModule = m_sample->wgpu.DeviceCreateShaderModule(m_sample->m_device, &vertexShaderModuleDescriptor);
        assert(vertWGSLShaderModule);

        WGPUShaderModuleWGSLDescriptor fragShaderModuleWGSLDescriptor{};
        fragShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = __shader_frag_wgsl.data(), .length = __shader_frag_wgsl.size() };

        WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
        fragShaderModuleDescriptor.nextInChain = &fragShaderModuleWGSLDescriptor.chain;

        fragWGSLShaderModule = m_sample->wgpu.DeviceCreateShaderModule(m_sample->m_device, &fragShaderModuleDescriptor);
        assert(fragWGSLShaderModule);
    }

    // create pipeline
    {

        WGPUPrimitiveState primitiveState{};
        primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
        primitiveState.cullMode = WGPUCullMode_None;
        primitiveState.frontFace = WGPUFrontFace_CCW;
        // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

        std::vector<WGPUVertexAttribute> attributes{};

        WGPUVertexAttribute positionAttribute{};
        positionAttribute.shaderLocation = 0;
        positionAttribute.format = WGPUVertexFormat_Float32x2;
        positionAttribute.offset = offsetof(ImDrawVert, pos);

        WGPUVertexAttribute uvAttribute{};
        uvAttribute.shaderLocation = 1;
        uvAttribute.format = WGPUVertexFormat_Float32x2;
        uvAttribute.offset = offsetof(ImDrawVert, uv);

        WGPUVertexAttribute colorAttribute{};
        colorAttribute.shaderLocation = 2;
        colorAttribute.format = WGPUVertexFormat_Unorm8x4;
        colorAttribute.offset = offsetof(ImDrawVert, col);

        attributes = { positionAttribute, uvAttribute, colorAttribute };

        std::vector<WGPUVertexBufferLayout> vertexBufferLayout(1);
        vertexBufferLayout[0].arrayStride = sizeof(ImDrawVert);
        vertexBufferLayout[0].stepMode = WGPUVertexStepMode_Vertex;
        vertexBufferLayout[0].attributeCount = attributes.size();
        vertexBufferLayout[0].attributes = attributes.data();

        std::string entryPoint = "main";
        WGPUVertexState vertexState{};
        vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
        vertexState.module = vertWGSLShaderModule;
        vertexState.bufferCount = static_cast<uint32_t>(vertexBufferLayout.size());
        vertexState.buffers = vertexBufferLayout.data();

        WGPUColorTargetState colorTargetState{};
        colorTargetState.format = m_sample->m_surfaceConfigure.format;
        colorTargetState.writeMask = WGPUColorWriteMask_All;

        WGPUBlendState blend{
            .color = {
                .srcFactor = WGPUBlendFactor_SrcAlpha,
                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                .operation = WGPUBlendOperation_Add,
            },
            .alpha = {
                .srcFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                .dstFactor = WGPUBlendFactor_Zero,
                .operation = WGPUBlendOperation_Add,
            }
        };
        colorTargetState.blend = &blend;

        WGPUFragmentState fragState{};
        fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
        fragState.module = fragWGSLShaderModule;
        fragState.targetCount = 1;
        fragState.targets = &colorTargetState;

        WGPUMultisampleState multisampleState{};
        multisampleState.count = 1;
        multisampleState.mask = 0xFFFFFFFF;

        WGPURenderPipelineDescriptor renderPipelineDescriptor{};
        renderPipelineDescriptor.layout = m_pipelineLayout;
        renderPipelineDescriptor.primitive = primitiveState;
        renderPipelineDescriptor.multisample = multisampleState;
        // renderPipelineDescriptor.depthStencil = &depthStencilState;
        renderPipelineDescriptor.vertex = vertexState;
        renderPipelineDescriptor.fragment = &fragState;

        m_pipeline = m_sample->wgpu.DeviceCreateRenderPipeline(m_sample->m_device, &renderPipelineDescriptor);
    }
}

void WGPUImGui::build()
{
    // update transfrom buffer
    {
        {
            ImDrawData* imDrawData = ImGui::GetDrawData();

            float L = imDrawData->DisplayPos.x;
            float R = imDrawData->DisplayPos.x + imDrawData->DisplaySize.x;
            float T = imDrawData->DisplayPos.y;
            float B = imDrawData->DisplayPos.y + imDrawData->DisplaySize.y;
            float mvp[4][4] = {
                { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
                { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.5f, 0.0f },
                { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
            };
            m_sample->wgpu.QueueWriteBuffer(m_sample->m_queue, m_uniformBuffer, offsetof(Uniform, mvp), mvp, sizeof(Uniform::mvp));
            float gamma;
            switch (m_sample->m_surfaceConfigure.format)
            {
            case WGPUTextureFormat_ASTC10x10UnormSrgb:
            case WGPUTextureFormat_ASTC10x5UnormSrgb:
            case WGPUTextureFormat_ASTC10x6UnormSrgb:
            case WGPUTextureFormat_ASTC10x8UnormSrgb:
            case WGPUTextureFormat_ASTC12x10UnormSrgb:
            case WGPUTextureFormat_ASTC12x12UnormSrgb:
            case WGPUTextureFormat_ASTC4x4UnormSrgb:
            case WGPUTextureFormat_ASTC5x5UnormSrgb:
            case WGPUTextureFormat_ASTC6x5UnormSrgb:
            case WGPUTextureFormat_ASTC6x6UnormSrgb:
            case WGPUTextureFormat_ASTC8x5UnormSrgb:
            case WGPUTextureFormat_ASTC8x6UnormSrgb:
            case WGPUTextureFormat_ASTC8x8UnormSrgb:
            case WGPUTextureFormat_BC1RGBAUnormSrgb:
            case WGPUTextureFormat_BC2RGBAUnormSrgb:
            case WGPUTextureFormat_BC3RGBAUnormSrgb:
            case WGPUTextureFormat_BC7RGBAUnormSrgb:
            case WGPUTextureFormat_BGRA8UnormSrgb:
            case WGPUTextureFormat_ETC2RGB8A1UnormSrgb:
            case WGPUTextureFormat_ETC2RGB8UnormSrgb:
            case WGPUTextureFormat_ETC2RGBA8UnormSrgb:
            case WGPUTextureFormat_RGBA8UnormSrgb:
                gamma = 2.2f;
                break;
            default:
                gamma = 1.0f;
            }
            m_sample->wgpu.QueueWriteBuffer(m_sample->m_queue, m_uniformBuffer, offsetof(Uniform, gamma), &gamma, sizeof(Uniform::gamma));
        }
    }

    // update draw buffer
    {
        ImDrawData* imDrawData = ImGui::GetDrawData();
        uint32_t vertexBufferSize = MEMALIGN(imDrawData->TotalVtxCount * sizeof(ImDrawVert), 4);
        uint32_t indexBufferSize = MEMALIGN(imDrawData->TotalIdxCount * sizeof(ImDrawIdx), 4);

        if ((vertexBufferSize == 0) || (indexBufferSize == 0))
        {
            return;
        }

        // Update buffers only if vertex or index count has been changed compared to current buffer size

        // Vertex buffer
        if ((m_vertexBuffer == nullptr) || (m_sample->wgpu.BufferGetSize(m_vertexBuffer) != vertexBufferSize))
        {
            WGPUBufferDescriptor descriptor{};
            descriptor.size = vertexBufferSize;
            descriptor.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
            // descriptor.mappedAtCreation = true;

            m_vertexBuffer = m_sample->wgpu.DeviceCreateBuffer(m_sample->m_device, &descriptor);
        }

        // Index buffer
        if ((m_indexBuffer == nullptr) || (m_sample->wgpu.BufferGetSize(m_indexBuffer) < indexBufferSize))
        {
            WGPUBufferDescriptor descriptor{};
            descriptor.size = indexBufferSize;
            descriptor.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
            // descriptor.mappedAtCreation = true;

            m_indexBuffer = m_sample->wgpu.DeviceCreateBuffer(m_sample->m_device, &descriptor);
        }

        // Upload data
        // ImDrawVert* vtxDst = (ImDrawVert*)m_sample->wgpu.BufferGetMappedRange(m_vertexBuffer, 0, vertexBufferSize);
        // ImDrawIdx* idxDst = (ImDrawIdx*)m_sample->wgpu.BufferGetMappedRange(m_indexBuffer, 0, indexBufferSize);

        int vertexOffset = 0;
        int indexOffset = 0;
        for (int n = 0; n < imDrawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[n];
            m_sample->wgpu.QueueWriteBuffer(m_sample->m_queue, m_vertexBuffer, vertexOffset, cmd_list->VtxBuffer.Data, MEMALIGN(cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), 4));
            m_sample->wgpu.QueueWriteBuffer(m_sample->m_queue, m_indexBuffer, indexOffset, cmd_list->IdxBuffer.Data, MEMALIGN(cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), 4));
            // memcpy(vtxDst, cmd_list->VtxBuffer.Data, MEMALIGN(cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), 4));
            // memcpy(idxDst, cmd_list->IdxBuffer.Data, MEMALIGN(cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), 4));
            // vtxDst += cmd_list->VtxBuffer.Size;
            // idxDst += cmd_list->IdxBuffer.Size;

            vertexOffset += cmd_list->VtxBuffer.Size;
            indexOffset += cmd_list->IdxBuffer.Size;
        }

        // m_sample->wgpu.BufferUnmap(m_vertexBuffer);
        // m_sample->wgpu.BufferUnmap(m_indexBuffer);

        // Flush if need
    }
}

void WGPUImGui::draw(WGPUCommandEncoder commandEncoder, WGPUTextureView renderView)
{
    ImDrawData* imDrawData = ImGui::GetDrawData();

    if (imDrawData && imDrawData->CmdListsCount > 0)
    {
        ImGuiIO& io = ImGui::GetIO();

        WGPURenderPassColorAttachment colorAttachment{};
        colorAttachment.view = renderView;
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        colorAttachment.loadOp = WGPULoadOp_Load;
        colorAttachment.storeOp = WGPUStoreOp_Store;
        colorAttachment.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

        WGPURenderPassDescriptor renderPassDescriptor{};
        renderPassDescriptor.colorAttachmentCount = 1;
        renderPassDescriptor.colorAttachments = &colorAttachment;

        WGPURenderPassEncoder renderPassEncoder = m_sample->wgpu.CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);
        m_sample->wgpu.RenderPassEncoderSetPipeline(renderPassEncoder, m_pipeline);
        m_sample->wgpu.RenderPassEncoderSetBindGroup(renderPassEncoder, 0, m_bindGroups[0], 0, nullptr);
        m_sample->wgpu.RenderPassEncoderSetBindGroup(renderPassEncoder, 1, m_bindGroups[1], 0, nullptr);
        m_sample->wgpu.RenderPassEncoderSetViewport(renderPassEncoder, 0.0f, 0.0f, static_cast<float>(m_sample->m_surfaceConfigure.width), static_cast<float>(m_sample->m_surfaceConfigure.height), 0.0f, 1.0f);
        m_sample->wgpu.RenderPassEncoderSetVertexBuffer(renderPassEncoder, 0, m_vertexBuffer, 0, 0);

        auto indexBufferSize = m_sample->wgpu.BufferGetSize(m_indexBuffer);
        m_sample->wgpu.RenderPassEncoderSetIndexBuffer(renderPassEncoder, m_indexBuffer, WGPUIndexFormat_Uint16, 0, indexBufferSize);

        int32_t vertexOffset = 0;
        int32_t indexOffset = 0;
        for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[i];
            for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                auto scissorX = std::max((int32_t)(pcmd->ClipRect.x), 0);
                auto scissorY = std::max((int32_t)(pcmd->ClipRect.y), 0);
                auto scissorWidth = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                auto scissorHeight = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);

                m_sample->wgpu.RenderPassEncoderSetScissorRect(renderPassEncoder, scissorX, scissorY, scissorWidth, scissorHeight);
                m_sample->wgpu.RenderPassEncoderDrawIndexed(renderPassEncoder, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
        m_sample->wgpu.RenderPassEncoderEnd(renderPassEncoder);
        m_sample->wgpu.RenderPassEncoderRelease(renderPassEncoder);
    }
}

void WGPUImGui::clear()
{
    // ImGui::DestroyContext();

    if (m_fontSampler)
    {
        m_sample->wgpu.SamplerRelease(m_fontSampler);
        m_fontSampler = nullptr;
    }

    if (m_fontTextureView)
    {
        m_sample->wgpu.TextureViewRelease(m_fontTextureView);
        m_fontTextureView = nullptr;
    }

    if (m_fontTexture)
    {
        m_sample->wgpu.TextureRelease(m_fontTexture);
        m_fontTexture = nullptr;
    }

    if (m_fontBuffer)
    {
        m_sample->wgpu.BufferRelease(m_fontBuffer);
        m_fontBuffer = nullptr;
    }

    if (m_uniformBuffer)
    {
        m_sample->wgpu.BufferRelease(m_uniformBuffer);
        m_uniformBuffer = nullptr;
    }

    if (m_vertexBuffer)
    {
        m_sample->wgpu.BufferRelease(m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }

    if (m_indexBuffer)
    {
        m_sample->wgpu.BufferRelease(m_indexBuffer);
        m_indexBuffer = nullptr;
    }

    if (m_pipeline)
    {
        m_sample->wgpu.RenderPipelineRelease(m_pipeline);
        m_pipeline = nullptr;
    }

    if (m_pipelineLayout)
    {
        m_sample->wgpu.PipelineLayoutRelease(m_pipelineLayout);
        m_pipelineLayout = nullptr;
    }

    for (auto bindGroup : m_bindGroups)
    {
        if (bindGroup)
        {
            m_sample->wgpu.BindGroupRelease(bindGroup);
        }
    }
    m_bindGroups.clear();

    for (auto bindGroupLayout : m_bindGroupLayouts)
    {
        if (bindGroupLayout)
        {
            m_sample->wgpu.BindGroupLayoutRelease(bindGroupLayout);
        }
    }
    m_bindGroupLayouts.clear();
}

void WGPUImGui::drawPolyline(std::string title, std::deque<float> data, std::string unit)
{
    if (data.empty())
        return;

    const auto size = data.size();
    const std::string description = fmt::format("{:.1f} {}", data[data.size() - 1], unit.c_str());
    int offset = 0;
    if (size > 15)
        offset = size - 15;
    ImGui::PlotLines(title.c_str(), &data[offset], size - offset, 0, description.c_str());
}

} // namespace jipu