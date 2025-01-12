#include "wgpu_imgui.h"

#include "wgpu_sample.h"
#include <fmt/format.h>
#include <stdexcept>

extern ImGuiID ImHashData(const void* data_p, size_t data_size, ImU32 seed = 0);
#define MEMALIGN(_SIZE, _ALIGN) (((_SIZE) + ((_ALIGN) - 1)) & ~((_ALIGN) - 1)) // Memory align (copied from IM_ALIGN() macro).

namespace jipu
{

void WGPUImGui::SafeRelease(ImDrawIdx*& res)
{
    if (res)
        delete[] res;
    res = nullptr;
}
void WGPUImGui::SafeRelease(ImDrawVert*& res)
{
    if (res)
        delete[] res;
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUBindGroupLayout& res)
{
    if (res)
        m_sample->wgpu.BindGroupLayoutRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUBindGroup& res)
{
    if (res)
        m_sample->wgpu.BindGroupRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUBuffer& res)
{
    if (res)
        m_sample->wgpu.BufferRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUPipelineLayout& res)
{
    if (res)
        m_sample->wgpu.PipelineLayoutRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPURenderPipeline& res)
{
    if (res)
        m_sample->wgpu.RenderPipelineRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUSampler& res)
{
    if (res)
        m_sample->wgpu.SamplerRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUShaderModule& res)
{
    if (res)
        m_sample->wgpu.ShaderModuleRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUTextureView& res)
{
    if (res)
        m_sample->wgpu.TextureViewRelease(res);
    res = nullptr;
}
void WGPUImGui::SafeRelease(WGPUTexture& res)
{
    if (res)
        m_sample->wgpu.TextureRelease(res);
    res = nullptr;
}

void WGPUImGui::SafeRelease(RenderResources& res)
{
    SafeRelease(res.FontTexture);
    SafeRelease(res.FontTextureView);
    SafeRelease(res.Sampler);
    SafeRelease(res.Uniforms);
    SafeRelease(res.CommonBindGroup);
    SafeRelease(res.ImageBindGroup);
    SafeRelease(res.ImageBindGroupLayout);
};

void WGPUImGui::SafeRelease(FrameResources& res)
{
    SafeRelease(res.IndexBuffer);
    SafeRelease(res.VertexBuffer);
    SafeRelease(res.IndexBufferHost);
    SafeRelease(res.VertexBufferHost);
}

static const char __shader_vert_wgsl[] = R"(
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

static const char __shader_frag_wgsl[] = R"(
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

WGPUImGui::~WGPUImGui()
{
    ImGui::DestroyContext();
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
    m_cmds.insert(m_cmds.end(), cmds.begin(), cmds.end());
}

void WGPUImGui::initialize()
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

    ImGui_ImplWGPU_InitInfo init_info;
    init_info.device = m_sample->m_device;
    init_info.numFramesInFlight = 3;
    init_info.renderTargetFormat = m_sample->m_surfaceConfigure.format;
    init_info.depthStencilFormat = WGPUTextureFormat_Undefined;

    init(&init_info);
}

void WGPUImGui::build()
{
    newFrame();

    ImGui::NewFrame();
    for (auto& cmd : m_cmds)
        cmd();
    ImGui::Render();

    m_cmds.clear();
}

void WGPUImGui::resize()
{
    invalidateDeviceObjects();
    createDeviceObjects();
}

void WGPUImGui::draw(WGPUCommandEncoder commandEncoder, WGPUTextureView renderView)
{
    WGPURenderPassColorAttachment color_attachments = {};
    color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attachments.loadOp = WGPULoadOp_Load;
    color_attachments.storeOp = WGPUStoreOp_Store;
    color_attachments.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f };
    color_attachments.view = renderView;

    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachments;
    render_pass_desc.depthStencilAttachment = nullptr;

    WGPURenderPassEncoder pass = m_sample->wgpu.CommandEncoderBeginRenderPass(commandEncoder, &render_pass_desc);
    renderDrawData(ImGui::GetDrawData(), pass);
    m_sample->wgpu.RenderPassEncoderEnd(pass);
    m_sample->wgpu.RenderPassEncoderRelease(pass);
}

void WGPUImGui::finalize()
{
    shutdown();
}

ImGui_ImplWGPU_Data* WGPUImGui::getBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplWGPU_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

WGPUProgrammableStageDescriptor WGPUImGui::createShaderModule(const char* wgsl_source)
{
    ImGui_ImplWGPU_Data* bd = getBackendData();

#ifdef USE_DAWN_HEADER
    WGPUShaderSourceWGSL wgsl_desc = {};
    wgsl_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl_desc.code = { wgsl_source, WGPU_STRLEN };
#else
    WGPUShaderModuleWGSLDescriptor wgsl_desc = {};
    wgsl_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    wgsl_desc.code = wgsl_source;
#endif

    WGPUShaderModuleDescriptor desc = {};
    desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl_desc);

    WGPUProgrammableStageDescriptor stage_desc = {};
    stage_desc.module = m_sample->wgpu.DeviceCreateShaderModule(bd->wgpuDevice, &desc);
#ifdef USE_DAWN_HEADER
    stage_desc.entryPoint = { "main", WGPU_STRLEN };
#else
    stage_desc.entryPoint = "main";
#endif
    return stage_desc;
}

WGPUBindGroup WGPUImGui::createImageBindGroup(WGPUBindGroupLayout layout, WGPUTextureView texture)
{
    ImGui_ImplWGPU_Data* bd = getBackendData();
    WGPUBindGroupEntry image_bg_entries[] = { { nullptr, 0, 0, 0, 0, 0, texture } };

    WGPUBindGroupDescriptor image_bg_descriptor = {};
    image_bg_descriptor.layout = layout;
    image_bg_descriptor.entryCount = sizeof(image_bg_entries) / sizeof(WGPUBindGroupEntry);
    image_bg_descriptor.entries = image_bg_entries;
    return m_sample->wgpu.DeviceCreateBindGroup(bd->wgpuDevice, &image_bg_descriptor);
}

void WGPUImGui::createFontsTexture()
{
    // Build texture atlas
    ImGui_ImplWGPU_Data* bd = getBackendData();
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height, size_pp;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &size_pp);

    // Upload texture to graphics system
    {
        WGPUTextureDescriptor tex_desc = {};
#ifdef USE_DAWN_HEADER
        tex_desc.label = { "Dear ImGui Font Texture", WGPU_STRLEN };
#else
        tex_desc.label = "Dear ImGui Font Texture";
#endif
        tex_desc.dimension = WGPUTextureDimension_2D;
        tex_desc.size.width = width;
        tex_desc.size.height = height;
        tex_desc.size.depthOrArrayLayers = 1;
        tex_desc.sampleCount = 1;
        tex_desc.format = WGPUTextureFormat_RGBA8Unorm;
        tex_desc.mipLevelCount = 1;
        tex_desc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding;
        bd->renderResources.FontTexture = m_sample->wgpu.DeviceCreateTexture(bd->wgpuDevice, &tex_desc);

        WGPUTextureViewDescriptor tex_view_desc = {};
        tex_view_desc.format = WGPUTextureFormat_RGBA8Unorm;
        tex_view_desc.dimension = WGPUTextureViewDimension_2D;
        tex_view_desc.baseMipLevel = 0;
        tex_view_desc.mipLevelCount = 1;
        tex_view_desc.baseArrayLayer = 0;
        tex_view_desc.arrayLayerCount = 1;
        tex_view_desc.aspect = WGPUTextureAspect_All;
        bd->renderResources.FontTextureView = m_sample->wgpu.TextureCreateView(bd->renderResources.FontTexture, &tex_view_desc);
    }

    // Upload texture data
    {
        WGPUImageCopyTexture dst_view = {};
        dst_view.texture = bd->renderResources.FontTexture;
        dst_view.mipLevel = 0;
        dst_view.origin = { 0, 0, 0 };
        dst_view.aspect = WGPUTextureAspect_All;
        WGPUTextureDataLayout layout = {};
        layout.offset = 0;
        layout.bytesPerRow = width * size_pp;
        layout.rowsPerImage = height;
        WGPUExtent3D size = { (uint32_t)width, (uint32_t)height, 1 };
        m_sample->wgpu.QueueWriteTexture(bd->defaultQueue, &dst_view, pixels, (uint32_t)(width * size_pp * height), &layout, &size);
    }

    // Create the associated sampler
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    {
        WGPUSamplerDescriptor sampler_desc = {};
        sampler_desc.minFilter = WGPUFilterMode_Linear;
        sampler_desc.magFilter = WGPUFilterMode_Linear;
        sampler_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
        sampler_desc.addressModeU = WGPUAddressMode_ClampToEdge;
        sampler_desc.addressModeV = WGPUAddressMode_ClampToEdge;
        sampler_desc.addressModeW = WGPUAddressMode_ClampToEdge;
        sampler_desc.maxAnisotropy = 1;
        bd->renderResources.Sampler = m_sample->wgpu.DeviceCreateSampler(bd->wgpuDevice, &sampler_desc);
    }

    // Store our identifier
    static_assert(sizeof(ImTextureID) >= sizeof(bd->renderResources.FontTexture), "Can't pack descriptor handle into TexID, 32-bit not supported yet.");
    io.Fonts->SetTexID((ImTextureID)bd->renderResources.FontTextureView);
}

void WGPUImGui::setupRenderState(ImDrawData* draw_data, WGPURenderPassEncoder ctx, FrameResources* fr)
{
    ImGui_ImplWGPU_Data* bd = getBackendData();

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    {
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        float mvp[4][4] = {
            { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
            { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.5f, 0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
        };
        m_sample->wgpu.QueueWriteBuffer(bd->defaultQueue, bd->renderResources.Uniforms, offsetof(Uniforms, MVP), mvp, sizeof(Uniforms::MVP));
        float gamma;
        switch (bd->renderTargetFormat)
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
        m_sample->wgpu.QueueWriteBuffer(bd->defaultQueue, bd->renderResources.Uniforms, offsetof(Uniforms, Gamma), &gamma, sizeof(Uniforms::Gamma));
    }

    // Setup viewport
    m_sample->wgpu.RenderPassEncoderSetViewport(ctx, 0, 0, draw_data->FramebufferScale.x * draw_data->DisplaySize.x, draw_data->FramebufferScale.y * draw_data->DisplaySize.y, 0, 1);

    // Bind shader and vertex buffers
    m_sample->wgpu.RenderPassEncoderSetVertexBuffer(ctx, 0, fr->VertexBuffer, 0, fr->VertexBufferSize * sizeof(ImDrawVert));
    m_sample->wgpu.RenderPassEncoderSetIndexBuffer(ctx, fr->IndexBuffer, sizeof(ImDrawIdx) == 2 ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32, 0, fr->IndexBufferSize * sizeof(ImDrawIdx));
    m_sample->wgpu.RenderPassEncoderSetPipeline(ctx, bd->pipelineState);
    m_sample->wgpu.RenderPassEncoderSetBindGroup(ctx, 0, bd->renderResources.CommonBindGroup, 0, nullptr);

    // Setup blend factor
    WGPUColor blend_color = { 0.f, 0.f, 0.f, 0.f };
    m_sample->wgpu.RenderPassEncoderSetBlendConstant(ctx, &blend_color);
}

void WGPUImGui::renderDrawData(ImDrawData* draw_data, WGPURenderPassEncoder pass_encoder)
{
    // Avoid rendering when minimized
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->CmdListsCount == 0)
        return;

    // FIXME: Assuming that this only gets called once per frame!
    // If not, we can't just re-allocate the IB or VB, we'll have to do a proper allocator.
    ImGui_ImplWGPU_Data* bd = getBackendData();
    bd->frameIndex = bd->frameIndex + 1;
    FrameResources* fr = &bd->pFrameResources[bd->frameIndex % bd->numFramesInFlight];

    // Create and grow vertex/index buffers if needed
    if (fr->VertexBuffer == nullptr || fr->VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (fr->VertexBuffer)
        {
            m_sample->wgpu.BufferDestroy(fr->VertexBuffer);
            m_sample->wgpu.BufferRelease(fr->VertexBuffer);
        }
        SafeRelease(fr->VertexBufferHost);
        fr->VertexBufferSize = draw_data->TotalVtxCount + 5000;

        WGPUBufferDescriptor vb_desc = {
            nullptr,
            "Dear ImGui Vertex buffer",
#ifdef USE_DAWN_HEADER
            WGPU_STRLEN,
#endif
            WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            MEMALIGN(fr->VertexBufferSize * sizeof(ImDrawVert), 4),
            false
        };
        fr->VertexBuffer = m_sample->wgpu.DeviceCreateBuffer(bd->wgpuDevice, &vb_desc);
        if (!fr->VertexBuffer)
            return;

        fr->VertexBufferHost = new ImDrawVert[fr->VertexBufferSize];
    }
    if (fr->IndexBuffer == nullptr || fr->IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (fr->IndexBuffer)
        {
            m_sample->wgpu.BufferDestroy(fr->IndexBuffer);
            m_sample->wgpu.BufferRelease(fr->IndexBuffer);
        }
        SafeRelease(fr->IndexBufferHost);
        fr->IndexBufferSize = draw_data->TotalIdxCount + 10000;

        WGPUBufferDescriptor ib_desc = {
            nullptr,
            "Dear ImGui Index buffer",
#ifdef USE_DAWN_HEADER
            WGPU_STRLEN,
#endif
            WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
            MEMALIGN(fr->IndexBufferSize * sizeof(ImDrawIdx), 4),
            false
        };
        fr->IndexBuffer = m_sample->wgpu.DeviceCreateBuffer(bd->wgpuDevice, &ib_desc);
        if (!fr->IndexBuffer)
            return;

        fr->IndexBufferHost = new ImDrawIdx[fr->IndexBufferSize];
    }

    // Upload vertex/index data into a single contiguous GPU buffer
    ImDrawVert* vtx_dst = (ImDrawVert*)fr->VertexBufferHost;
    ImDrawIdx* idx_dst = (ImDrawIdx*)fr->IndexBufferHost;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += draw_list->VtxBuffer.Size;
        idx_dst += draw_list->IdxBuffer.Size;
    }
    int64_t vb_write_size = MEMALIGN((char*)vtx_dst - (char*)fr->VertexBufferHost, 4);
    int64_t ib_write_size = MEMALIGN((char*)idx_dst - (char*)fr->IndexBufferHost, 4);
    m_sample->wgpu.QueueWriteBuffer(bd->defaultQueue, fr->VertexBuffer, 0, fr->VertexBufferHost, vb_write_size);
    m_sample->wgpu.QueueWriteBuffer(bd->defaultQueue, fr->IndexBuffer, 0, fr->IndexBufferHost, ib_write_size);

    // Setup desired render state
    setupRenderState(draw_data, pass_encoder, fr);

    // Setup render state structure (for callbacks and custom texture bindings)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    ImGui_ImplWGPU_RenderState render_state;
    render_state.Device = bd->wgpuDevice;
    render_state.RenderPassEncoder = pass_encoder;
    platform_io.Renderer_RenderState = &render_state;

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_scale = draw_data->FramebufferScale;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    setupRenderState(draw_data, pass_encoder, fr);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            }
            else
            {
                // Bind custom texture
                ImTextureID tex_id = pcmd->GetTexID();
                ImGuiID tex_id_hash = ImHashData(&tex_id, sizeof(tex_id));
                auto bind_group = bd->renderResources.ImageBindGroups.GetVoidPtr(tex_id_hash);
                if (bind_group)
                {
                    m_sample->wgpu.RenderPassEncoderSetBindGroup(pass_encoder, 1, (WGPUBindGroup)bind_group, 0, nullptr);
                }
                else
                {
                    WGPUBindGroup image_bind_group = createImageBindGroup(bd->renderResources.ImageBindGroupLayout, (WGPUTextureView)tex_id);
                    bd->renderResources.ImageBindGroups.SetVoidPtr(tex_id_hash, image_bind_group);
                    m_sample->wgpu.RenderPassEncoderSetBindGroup(pass_encoder, 1, image_bind_group, 0, nullptr);
                }

                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as wgpuRenderPassEncoderSetScissorRect() won't accept values that are off bounds
                if (clip_min.x < 0.0f)
                {
                    clip_min.x = 0.0f;
                }
                if (clip_min.y < 0.0f)
                {
                    clip_min.y = 0.0f;
                }
                if (clip_max.x > fb_width)
                {
                    clip_max.x = (float)fb_width;
                }
                if (clip_max.y > fb_height)
                {
                    clip_max.y = (float)fb_height;
                }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle, Draw
                m_sample->wgpu.RenderPassEncoderSetScissorRect(pass_encoder, (uint32_t)clip_min.x, (uint32_t)clip_min.y, (uint32_t)(clip_max.x - clip_min.x), (uint32_t)(clip_max.y - clip_min.y));
                m_sample->wgpu.RenderPassEncoderDrawIndexed(pass_encoder, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += draw_list->IdxBuffer.Size;
        global_vtx_offset += draw_list->VtxBuffer.Size;
    }
    platform_io.Renderer_RenderState = nullptr;
}

void WGPUImGui::createUniformBuffer()
{
    ImGui_ImplWGPU_Data* bd = getBackendData();
    WGPUBufferDescriptor ub_desc = {
        nullptr,
        "Dear ImGui Uniform buffer",
#ifdef USE_DAWN_HEADER
        WGPU_STRLEN,
#endif
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        MEMALIGN(sizeof(Uniforms), 16),
        false
    };
    bd->renderResources.Uniforms = m_sample->wgpu.DeviceCreateBuffer(bd->wgpuDevice, &ub_desc);
}

bool WGPUImGui::createDeviceObjects()
{
    ImGui_ImplWGPU_Data* bd = getBackendData();
    if (!bd->wgpuDevice)
        return false;
    if (bd->pipelineState)
        invalidateDeviceObjects();

    // Create render pipeline
    WGPURenderPipelineDescriptor graphics_pipeline_desc = {};
    graphics_pipeline_desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    graphics_pipeline_desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    graphics_pipeline_desc.primitive.frontFace = WGPUFrontFace_CW;
    graphics_pipeline_desc.primitive.cullMode = WGPUCullMode_None;
    graphics_pipeline_desc.multisample = bd->initInfo.pipelineMultisampleState;

    // Bind group layouts
    WGPUBindGroupLayoutEntry common_bg_layout_entries[2] = {};
    common_bg_layout_entries[0].binding = 0;
    common_bg_layout_entries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    common_bg_layout_entries[0].buffer.type = WGPUBufferBindingType_Uniform;
    common_bg_layout_entries[1].binding = 1;
    common_bg_layout_entries[1].visibility = WGPUShaderStage_Fragment;
    common_bg_layout_entries[1].sampler.type = WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutEntry image_bg_layout_entries[1] = {};
    image_bg_layout_entries[0].binding = 0;
    image_bg_layout_entries[0].visibility = WGPUShaderStage_Fragment;
    image_bg_layout_entries[0].texture.sampleType = WGPUTextureSampleType_Float;
    image_bg_layout_entries[0].texture.viewDimension = WGPUTextureViewDimension_2D;

    WGPUBindGroupLayoutDescriptor common_bg_layout_desc = {};
    common_bg_layout_desc.entryCount = 2;
    common_bg_layout_desc.entries = common_bg_layout_entries;

    WGPUBindGroupLayoutDescriptor image_bg_layout_desc = {};
    image_bg_layout_desc.entryCount = 1;
    image_bg_layout_desc.entries = image_bg_layout_entries;

    WGPUBindGroupLayout bg_layouts[2];
    bg_layouts[0] = m_sample->wgpu.DeviceCreateBindGroupLayout(bd->wgpuDevice, &common_bg_layout_desc);
    bg_layouts[1] = m_sample->wgpu.DeviceCreateBindGroupLayout(bd->wgpuDevice, &image_bg_layout_desc);

    WGPUPipelineLayoutDescriptor layout_desc = {};
    layout_desc.bindGroupLayoutCount = 2;
    layout_desc.bindGroupLayouts = bg_layouts;
    graphics_pipeline_desc.layout = m_sample->wgpu.DeviceCreatePipelineLayout(bd->wgpuDevice, &layout_desc);

    // Create the vertex shader
    WGPUProgrammableStageDescriptor vertex_shader_desc = createShaderModule(__shader_vert_wgsl);
    graphics_pipeline_desc.vertex.module = vertex_shader_desc.module;
    graphics_pipeline_desc.vertex.entryPoint = vertex_shader_desc.entryPoint;

    // Vertex input configuration
    WGPUVertexAttribute attribute_desc[] = {
        { WGPUVertexFormat_Float32x2, (uint64_t)offsetof(ImDrawVert, pos), 0 },
        { WGPUVertexFormat_Float32x2, (uint64_t)offsetof(ImDrawVert, uv), 1 },
        { WGPUVertexFormat_Unorm8x4, (uint64_t)offsetof(ImDrawVert, col), 2 },
    };

    WGPUVertexBufferLayout buffer_layouts[1];
    buffer_layouts[0].arrayStride = sizeof(ImDrawVert);
    buffer_layouts[0].stepMode = WGPUVertexStepMode_Vertex;
    buffer_layouts[0].attributeCount = 3;
    buffer_layouts[0].attributes = attribute_desc;

    graphics_pipeline_desc.vertex.bufferCount = 1;
    graphics_pipeline_desc.vertex.buffers = buffer_layouts;

    // Create the pixel shader
    WGPUProgrammableStageDescriptor pixel_shader_desc = createShaderModule(__shader_frag_wgsl);

    // Create the blending setup
    WGPUBlendState blend_state = {};
    blend_state.alpha.operation = WGPUBlendOperation_Add;
    blend_state.alpha.srcFactor = WGPUBlendFactor_One;
    blend_state.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend_state.color.operation = WGPUBlendOperation_Add;
    blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

    WGPUColorTargetState color_state = {};
    color_state.format = bd->renderTargetFormat;
    color_state.blend = &blend_state;
    color_state.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragment_state = {};
    fragment_state.module = pixel_shader_desc.module;
    fragment_state.entryPoint = pixel_shader_desc.entryPoint;
    fragment_state.targetCount = 1;
    fragment_state.targets = &color_state;

    graphics_pipeline_desc.fragment = &fragment_state;

    // Create depth-stencil State
    WGPUDepthStencilState depth_stencil_state = {};
    depth_stencil_state.format = bd->depthStencilFormat;
#ifdef USE_DAWN_HEADER
    depth_stencil_state.depthWriteEnabled = WGPUOptionalBool_False;
#else
    depth_stencil_state.depthWriteEnabled = false;
#endif
    depth_stencil_state.depthCompare = WGPUCompareFunction_Always;
    depth_stencil_state.stencilFront.compare = WGPUCompareFunction_Always;
    depth_stencil_state.stencilFront.failOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilFront.depthFailOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilFront.passOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilBack.compare = WGPUCompareFunction_Always;
    depth_stencil_state.stencilBack.failOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilBack.depthFailOp = WGPUStencilOperation_Keep;
    depth_stencil_state.stencilBack.passOp = WGPUStencilOperation_Keep;

    // Configure disabled depth-stencil state
    graphics_pipeline_desc.depthStencil = (bd->depthStencilFormat == WGPUTextureFormat_Undefined) ? nullptr : &depth_stencil_state;

    bd->pipelineState = m_sample->wgpu.DeviceCreateRenderPipeline(bd->wgpuDevice, &graphics_pipeline_desc);

    createFontsTexture();
    createUniformBuffer();

    // Create resource bind group
    WGPUBindGroupEntry common_bg_entries[] = {
        { nullptr, 0, bd->renderResources.Uniforms, 0, MEMALIGN(sizeof(Uniforms), 16), 0, 0 },
        { nullptr, 1, 0, 0, 0, bd->renderResources.Sampler, 0 },
    };

    WGPUBindGroupDescriptor common_bg_descriptor = {};
    common_bg_descriptor.layout = bg_layouts[0];
    common_bg_descriptor.entryCount = sizeof(common_bg_entries) / sizeof(WGPUBindGroupEntry);
    common_bg_descriptor.entries = common_bg_entries;
    bd->renderResources.CommonBindGroup = m_sample->wgpu.DeviceCreateBindGroup(bd->wgpuDevice, &common_bg_descriptor);

    WGPUBindGroup image_bind_group = createImageBindGroup(bg_layouts[1], bd->renderResources.FontTextureView);
    bd->renderResources.ImageBindGroup = image_bind_group;
    bd->renderResources.ImageBindGroupLayout = bg_layouts[1];
    bd->renderResources.ImageBindGroups.SetVoidPtr(ImHashData(&bd->renderResources.FontTextureView, sizeof(ImTextureID)), image_bind_group);

    SafeRelease(vertex_shader_desc.module);
    SafeRelease(pixel_shader_desc.module);
    SafeRelease(graphics_pipeline_desc.layout);
    SafeRelease(bg_layouts[0]);

    return true;
}

void WGPUImGui::invalidateDeviceObjects()
{
    ImGui_ImplWGPU_Data* bd = getBackendData();
    if (!bd->wgpuDevice)
        return;

    SafeRelease(bd->pipelineState);
    SafeRelease(bd->renderResources);

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->SetTexID(0); // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.

    for (unsigned int i = 0; i < bd->numFramesInFlight; i++)
        SafeRelease(bd->pFrameResources[i]);
}

bool WGPUImGui::init(ImGui_ImplWGPU_InitInfo* init_info)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplWGPU_Data* bd = IM_NEW(ImGui_ImplWGPU_Data)();
    io.BackendRendererUserData = (void*)bd;
#if defined(__EMSCRIPTEN__)
    io.BackendRendererName = "imgui_impl_webgpu_emscripten";
#elif defined(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    io.BackendRendererName = "imgui_impl_webgpu_dawn";
#elif defined(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
    io.BackendRendererName = "imgui_impl_webgpu_wgpu";
#else
    io.BackendRendererName = "imgui_impl_webgpu";
#endif
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    bd->initInfo = *init_info;
    bd->wgpuDevice = init_info->device;
    bd->defaultQueue = m_sample->wgpu.DeviceGetQueue(bd->wgpuDevice);
    bd->renderTargetFormat = init_info->renderTargetFormat;
    bd->depthStencilFormat = init_info->depthStencilFormat;
    bd->numFramesInFlight = init_info->numFramesInFlight;
    bd->frameIndex = UINT_MAX;

    bd->renderResources.FontTexture = nullptr;
    bd->renderResources.FontTextureView = nullptr;
    bd->renderResources.Sampler = nullptr;
    bd->renderResources.Uniforms = nullptr;
    bd->renderResources.CommonBindGroup = nullptr;
    bd->renderResources.ImageBindGroups.Data.reserve(100);
    bd->renderResources.ImageBindGroup = nullptr;
    bd->renderResources.ImageBindGroupLayout = nullptr;

    // Create buffers with a default size (they will later be grown as needed)
    bd->pFrameResources = new FrameResources[bd->numFramesInFlight];
    for (unsigned int i = 0; i < bd->numFramesInFlight; i++)
    {
        FrameResources* fr = &bd->pFrameResources[i];
        fr->IndexBuffer = nullptr;
        fr->VertexBuffer = nullptr;
        fr->IndexBufferHost = nullptr;
        fr->VertexBufferHost = nullptr;
        fr->IndexBufferSize = 10000;
        fr->VertexBufferSize = 5000;
    }

    return true;
}

void WGPUImGui::shutdown()
{
    ImGui_ImplWGPU_Data* bd = getBackendData();
    // IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    if (!bd)
        return;

    ImGuiIO& io = ImGui::GetIO();

    invalidateDeviceObjects();
    delete[] bd->pFrameResources;
    bd->pFrameResources = nullptr;
    // m_sample->wgpu.QueueRelease(bd->defaultQueue);
    bd->defaultQueue = nullptr;
    bd->wgpuDevice = nullptr;
    bd->numFramesInFlight = 0;
    bd->frameIndex = UINT_MAX;

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

void WGPUImGui::newFrame()
{
    ImGui_ImplWGPU_Data* bd = getBackendData();
    if (!bd->pipelineState)
        createDeviceObjects();
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