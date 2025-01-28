#include "native_imgui.h"

#include <fmt/format.h>
#include <stdexcept>
#include <string_view>

namespace jipu
{

/*
#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

layout(set = 0, binding = 0) uniform UITransform {
    vec2 scale;
    vec2 translate;
} uiTransform;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    outUV = inUV;
    outColor = inColor;
    gl_Position = vec4(inPos * uiTransform.scale + uiTransform.translate, 0.0, 1.0);
}
*/
static std::vector<uint32_t> vertexShaderSourceSpv = { 0x07230203, 0x00010000, 0x000d000b, 0x0000002b, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x000b000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000b, 0x0000000f, 0x00000011, 0x00000015, 0x00000018, 0x00030003, 0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365, 0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572, 0x00657669, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x00000009, 0x5574756f, 0x00000056, 0x00040005, 0x0000000b, 0x56556e69, 0x00000000, 0x00050005, 0x0000000f, 0x4374756f, 0x726f6c6f, 0x00000000, 0x00040005, 0x00000011, 0x6f436e69, 0x00726f6c, 0x00060005, 0x00000013, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000013, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x00000015, 0x00000000, 0x00040005, 0x00000018, 0x6f506e69, 0x00000073, 0x00050005, 0x0000001a, 0x72544955, 0x66736e61, 0x006d726f, 0x00050006, 0x0000001a, 0x00000000, 0x6c616373, 0x00000065, 0x00060006, 0x0000001a, 0x00000001, 0x6e617274, 0x74616c73, 0x00000065, 0x00050005, 0x0000001c, 0x72546975, 0x66736e61, 0x006d726f, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000b, 0x0000001e, 0x00000001, 0x00040047, 0x0000000f, 0x0000001e, 0x00000001, 0x00040047, 0x00000011, 0x0000001e, 0x00000002, 0x00050048, 0x00000013, 0x00000000, 0x0000000b, 0x00000000, 0x00030047, 0x00000013, 0x00000002, 0x00040047, 0x00000018, 0x0000001e, 0x00000000, 0x00050048, 0x0000001a, 0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x0000001a, 0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x0000001a, 0x00000002, 0x00040047, 0x0000001c, 0x00000022, 0x00000000, 0x00040047, 0x0000001c, 0x00000021, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000002, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040020, 0x0000000a, 0x00000001, 0x00000007, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000001, 0x00040017, 0x0000000d, 0x00000006, 0x00000004, 0x00040020, 0x0000000e, 0x00000003, 0x0000000d, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000003, 0x00040020, 0x00000010, 0x00000001, 0x0000000d, 0x0004003b, 0x00000010, 0x00000011, 0x00000001, 0x0003001e, 0x00000013, 0x0000000d, 0x00040020, 0x00000014, 0x00000003, 0x00000013, 0x0004003b, 0x00000014, 0x00000015, 0x00000003, 0x00040015, 0x00000016, 0x00000020, 0x00000001, 0x0004002b, 0x00000016, 0x00000017, 0x00000000, 0x0004003b, 0x0000000a, 0x00000018, 0x00000001, 0x0004001e, 0x0000001a, 0x00000007, 0x00000007, 0x00040020, 0x0000001b, 0x00000002, 0x0000001a, 0x0004003b, 0x0000001b, 0x0000001c, 0x00000002, 0x00040020, 0x0000001d, 0x00000002, 0x00000007, 0x0004002b, 0x00000016, 0x00000021, 0x00000001, 0x0004002b, 0x00000006, 0x00000025, 0x00000000, 0x0004002b, 0x00000006, 0x00000026, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000007, 0x0000000c, 0x0000000b, 0x0003003e, 0x00000009, 0x0000000c, 0x0004003d, 0x0000000d, 0x00000012, 0x00000011, 0x0003003e, 0x0000000f, 0x00000012, 0x0004003d, 0x00000007, 0x00000019, 0x00000018, 0x00050041, 0x0000001d, 0x0000001e, 0x0000001c, 0x00000017, 0x0004003d, 0x00000007, 0x0000001f, 0x0000001e, 0x00050085, 0x00000007, 0x00000020, 0x00000019, 0x0000001f, 0x00050041, 0x0000001d, 0x00000022, 0x0000001c, 0x00000021, 0x0004003d, 0x00000007, 0x00000023, 0x00000022, 0x00050081, 0x00000007, 0x00000024, 0x00000020, 0x00000023, 0x00050051, 0x00000006, 0x00000027, 0x00000024, 0x00000000, 0x00050051, 0x00000006, 0x00000028, 0x00000024, 0x00000001, 0x00070050, 0x0000000d, 0x00000029, 0x00000027, 0x00000028, 0x00000025, 0x00000026, 0x00050041, 0x0000000e, 0x0000002a, 0x00000015, 0x00000017, 0x0003003e, 0x0000002a, 0x00000029, 0x000100fd, 0x00010038 };

/*
#version 450

layout(set = 1, binding = 0) uniform sampler fontSampler;
layout(set = 1, binding = 1) uniform texture2D fontTexture;

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = inColor * texture(sampler2D(fontTexture, fontSampler), inUV);
}
*/

static std::vector<uint32_t> fragmentShaderSourceSpv = { 0x07230203, 0x00010000, 0x000d000b, 0x0000001d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0008000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000b, 0x00000019, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x000a0004, 0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365, 0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572, 0x00657669, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000, 0x00040005, 0x0000000b, 0x6f436e69, 0x00726f6c, 0x00050005, 0x0000000f, 0x746e6f66, 0x74786554, 0x00657275, 0x00050005, 0x00000013, 0x746e6f66, 0x706d6153, 0x0072656c, 0x00040005, 0x00000019, 0x56556e69, 0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000b, 0x0000001e, 0x00000001, 0x00040047, 0x0000000f, 0x00000022, 0x00000001, 0x00040047, 0x0000000f, 0x00000021, 0x00000001, 0x00040047, 0x00000013, 0x00000022, 0x00000001, 0x00040047, 0x00000013, 0x00000021, 0x00000000, 0x00040047, 0x00000019, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040020, 0x0000000a, 0x00000001, 0x00000007, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000001, 0x00090019, 0x0000000d, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00040020, 0x0000000e, 0x00000000, 0x0000000d, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000000, 0x0002001a, 0x00000011, 0x00040020, 0x00000012, 0x00000000, 0x00000011, 0x0004003b, 0x00000012, 0x00000013, 0x00000000, 0x0003001b, 0x00000015, 0x0000000d, 0x00040017, 0x00000017, 0x00000006, 0x00000002, 0x00040020, 0x00000018, 0x00000001, 0x00000017, 0x0004003b, 0x00000018, 0x00000019, 0x00000001, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000007, 0x0000000c, 0x0000000b, 0x0004003d, 0x0000000d, 0x00000010, 0x0000000f, 0x0004003d, 0x00000011, 0x00000014, 0x00000013, 0x00050056, 0x00000015, 0x00000016, 0x00000010, 0x00000014, 0x0004003d, 0x00000017, 0x0000001a, 0x00000019, 0x00050057, 0x00000007, 0x0000001b, 0x00000016, 0x0000001a, 0x00050085, 0x00000007, 0x0000001c, 0x0000000c, 0x0000001b, 0x0003003e, 0x00000009, 0x0000001c, 0x000100fd, 0x00010038 };

void NativeImGui::window(const char* title, std::vector<std::function<void()>> uis)
{
    // set windows position and size
    {
        auto scale = ImGui::GetIO().FontGlobalScale;
        ImGui::SetNextWindowPos(ImVec2(20, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300 * scale, 100 * scale), ImGuiCond_FirstUseEver);
    }

    ImGui::Begin(title);
    for (auto& ui : uis)
        ui();
    ImGui::End();
}

void NativeImGui::record(std::vector<std::function<void()>> cmds)
{
    ImGui::NewFrame();
    for (auto& cmd : cmds)
        cmd();
    ImGui::Render();
}

void NativeImGui::init(Device* device, Queue* queue, Swapchain* swapchain)
{
    m_device = device;
    m_queue = queue;

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
    io.DisplaySize = ImVec2(swapchain->getWidth(), swapchain->getHeight());
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
        TextureDescriptor fontTextureDescriptor{};
        fontTextureDescriptor.type = TextureType::k2D;
        fontTextureDescriptor.format = TextureFormat::kRGBA8Unorm;
        fontTextureDescriptor.width = fontTexWidth;
        fontTextureDescriptor.height = fontTexHeight;
        fontTextureDescriptor.depth = 1;
        fontTextureDescriptor.mipLevels = 1;
        fontTextureDescriptor.sampleCount = 1;
        fontTextureDescriptor.usage = TextureUsageFlagBits::kCopyDst |
                                      TextureUsageFlagBits::kTextureBinding;

        m_fontTexture = device->createTexture(fontTextureDescriptor);
    }

    // create font texture view.
    {
        TextureViewDescriptor fontTextureViewDescriptor{};
        fontTextureViewDescriptor.aspect = TextureAspectFlagBits::kColor;
        fontTextureViewDescriptor.dimension = TextureViewDimension::k2D;

        m_fontTextureView = m_fontTexture->createTextureView(fontTextureViewDescriptor);
    }

    // create font staging buffer.
    {
        BufferDescriptor fontBufferDescriptor{};
        fontBufferDescriptor.size = fontTexWidth * fontTexHeight * 4 * sizeof(FontDataType);
        fontBufferDescriptor.usage = BufferUsageFlagBits::kCopySrc;

        m_fontBuffer = device->createBuffer(fontBufferDescriptor);

        void* fontGPUMappedPointer = m_fontBuffer->map();
        memcpy(fontGPUMappedPointer, fontData, fontBufferDescriptor.size);
        m_fontBuffer->unmap();
    }

    // copy buffer to texture
    {
        CopyTextureBuffer copyTextureBuffer{
            .buffer = m_fontBuffer.get(),
            .offset = 0,
            .bytesPerRow = 0,
            .rowsPerTexture = 0,
        };

        uint32_t channel = 4;
        uint32_t bytesPerData = sizeof(FontDataType);
        copyTextureBuffer.bytesPerRow = bytesPerData * m_fontTexture->getWidth() * channel;
        copyTextureBuffer.rowsPerTexture = m_fontTexture->getHeight();

        CopyTexture copyTexture{
            .texture = m_fontTexture.get(),
            .aspect = TextureAspectFlagBits::kColor
        };
        Extent3D extent{};
        extent.width = m_fontTexture->getWidth();
        extent.height = m_fontTexture->getHeight();
        extent.depth = 1;

        CommandEncoderDescriptor commandEncoderDescriptor{};
        std::unique_ptr<CommandEncoder> commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);
        commandEncoder->copyBufferToTexture(copyTextureBuffer, copyTexture, extent);

        auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
        queue->submit({ commandBuffer.get() });
    }

    // create uniform buffer
    {
        BufferDescriptor uniformBufferDescriptor{};
        uniformBufferDescriptor.size = sizeof(UITransform);
        uniformBufferDescriptor.usage = BufferUsageFlagBits::kUniform;

        m_uniformBuffer = m_device->createBuffer(uniformBufferDescriptor);
    }

    // create font sampler
    {
        SamplerDescriptor fontSamplerDescriptor{};
        fontSamplerDescriptor.addressModeU = AddressMode::kClampToEdge;
        fontSamplerDescriptor.addressModeV = AddressMode::kClampToEdge;
        fontSamplerDescriptor.addressModeW = AddressMode::kClampToEdge;
        fontSamplerDescriptor.lodMin = 0.0f;
        fontSamplerDescriptor.lodMax = static_cast<uint32_t>(m_fontTexture->getMipLevels());
        fontSamplerDescriptor.minFilter = FilterMode::kLinear;
        fontSamplerDescriptor.magFilter = FilterMode::kLinear;
        fontSamplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;

        m_fontSampler = device->createSampler(fontSamplerDescriptor);
    }

    // create binding group layout
    {
        m_bindGroupLayouts.resize(2);
        {
            BufferBindingLayout uiTransformBindingLayout{};
            uiTransformBindingLayout.index = 0;
            uiTransformBindingLayout.stages = BindingStageFlagBits::kVertexStage;
            uiTransformBindingLayout.type = BufferBindingType::kUniform;

            BindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
            bindGroupLayoutDescriptor.buffers = { uiTransformBindingLayout };

            m_bindGroupLayouts[0] = device->createBindGroupLayout(bindGroupLayoutDescriptor);
        }
        {
            SamplerBindingLayout fontSamplerBindingLayout{};
            fontSamplerBindingLayout.index = 0;
            fontSamplerBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

            TextureBindingLayout fontTextureBindingLayout{};
            fontTextureBindingLayout.index = 1;
            fontTextureBindingLayout.stages = BindingStageFlagBits::kFragmentStage;

            BindGroupLayoutDescriptor bindGroupLayoutDescriptor{};
            bindGroupLayoutDescriptor.samplers = { fontSamplerBindingLayout };
            bindGroupLayoutDescriptor.textures = { fontTextureBindingLayout };

            m_bindGroupLayouts[1] = device->createBindGroupLayout(bindGroupLayoutDescriptor);
        }
    }

    // create binding group
    {
        m_bindGroups.resize(2);
        {
            BufferBinding uiTransformBinding{
                .index = 0,
                .offset = 0,
                .size = m_uniformBuffer->getSize(),
                .buffer = m_uniformBuffer.get(),
            };

            BindGroupDescriptor bindGroupDescriptor{
                .layout = m_bindGroupLayouts[0].get(),
                .buffers = { uiTransformBinding },
            };

            m_bindGroups[0] = device->createBindGroup(bindGroupDescriptor);
        }

        {
            SamplerBinding fontSamplerBinding{
                .index = 0,
                .sampler = m_fontSampler.get(),
            };

            TextureBinding fontTextureBinding{
                .index = 1,
                .textureView = m_fontTextureView.get(),
            };

            BindGroupDescriptor bindGroupDescriptor{
                .layout = m_bindGroupLayouts[1].get(),
                .samplers = { fontSamplerBinding },
                .textures = { fontTextureBinding },
            };

            m_bindGroups[1] = device->createBindGroup(bindGroupDescriptor);
        }
    }

    // create pipeline layout
    {
        PipelineLayoutDescriptor pipelineLayoutDescriptor{};
        pipelineLayoutDescriptor.layouts = { m_bindGroupLayouts[0].get(), m_bindGroupLayouts[1].get() };

        m_pipelineLayout = device->createPipelineLayout(pipelineLayoutDescriptor);
    }

    // create pipeline
    {
        InputAssemblyStage inputAssemblyStage{};
        {
            inputAssemblyStage.topology = PrimitiveTopology::kTriangleList;
        }

        std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;

        VertexInputLayout vertexInputLayout{};
        {
            VertexAttribute positionAttribute{};
            positionAttribute.format = VertexFormat::kFloat32x2;
            positionAttribute.offset = offsetof(ImDrawVert, pos);
            positionAttribute.location = 0;

            VertexAttribute uiAttribute{};
            uiAttribute.format = VertexFormat::kFloat32x2;
            uiAttribute.offset = offsetof(ImDrawVert, uv);
            uiAttribute.location = 1;

            VertexAttribute colorAttribute{};
            colorAttribute.format = VertexFormat::kUnorm8x4;
            colorAttribute.offset = offsetof(ImDrawVert, col);
            colorAttribute.location = 2;

            vertexInputLayout.attributes = { positionAttribute, uiAttribute, colorAttribute };
            vertexInputLayout.mode = VertexMode::kVertex;
            vertexInputLayout.stride = sizeof(ImDrawVert);
        }

        {
            std::vector<char> vertexShaderSource(vertexShaderSourceSpv.size() * sizeof(uint32_t));
            std::memcpy(vertexShaderSource.data(), vertexShaderSourceSpv.data(), vertexShaderSource.size());

            ShaderModuleDescriptor vertexShaderModuleDescriptor{};
            vertexShaderModuleDescriptor.type = ShaderModuleType::kSPIRV;
            vertexShaderModuleDescriptor.code = std::string_view(vertexShaderSource.data(), vertexShaderSource.size());

            vertexShaderModule = device->createShaderModule(vertexShaderModuleDescriptor);
        }

        VertexStage vertexStage{
            { vertexShaderModule.get(), "main" },
            { vertexInputLayout },
        };

        RasterizationStage rasterizationStage{};
        {
            rasterizationStage.cullMode = CullMode::kNone;
            rasterizationStage.frontFace = FrontFace::kCounterClockwise;
            rasterizationStage.sampleCount = 1;
        }

        std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;

        FragmentStage::Target target{};
        target.format = swapchain->getTextureFormat();
        target.blend = {
            .color = {
                .srcFactor = BlendFactor::kSrcAlpha,
                .dstFactor = BlendFactor::kOneMinusSrcAlpha,
                .operation = BlendOperation::kAdd,
            },
            .alpha = {

                .srcFactor = BlendFactor::kOneMinusSrcAlpha,
                .dstFactor = BlendFactor::kZero,
                .operation = BlendOperation::kAdd,
            }
        };

        {

            std::vector<char> fragmentShaderSource(fragmentShaderSourceSpv.size() * sizeof(uint32_t));
            std::memcpy(fragmentShaderSource.data(), fragmentShaderSourceSpv.data(), fragmentShaderSource.size());

            ShaderModuleDescriptor fragmentShaderModuleDescriptor{};

            fragmentShaderModuleDescriptor.type = ShaderModuleType::kSPIRV;
            fragmentShaderModuleDescriptor.code = std::string_view(fragmentShaderSource.data(), fragmentShaderSource.size());

            fragmentShaderModule = device->createShaderModule(fragmentShaderModuleDescriptor);
        }

        FragmentStage fragmentStage{
            { fragmentShaderModule.get(), "main" }, // ProgramableStage
            { target }
        };

        RenderPipelineDescriptor renderPipelineDescriptor{
            m_pipelineLayout.get(), // PipelineDescriptor
            inputAssemblyStage,
            vertexStage,
            rasterizationStage,
            fragmentStage,
        };

        m_pipeline = device->createRenderPipeline(renderPipelineDescriptor);
    }
}

void NativeImGui::build()
{
    // update transfrom buffer
    {
        float L = ImGui::GetDrawData()->DisplayPos.x;
        float R = ImGui::GetDrawData()->DisplayPos.x + ImGui::GetDrawData()->DisplaySize.x;
        float T = ImGui::GetDrawData()->DisplayPos.y;
        float B = ImGui::GetDrawData()->DisplayPos.y + ImGui::GetDrawData()->DisplaySize.y;

        m_uiTransform.scale[0] = 2.0f / (R - L);
        m_uiTransform.scale[1] = 2.0f / (T - B);

        m_uiTransform.translate[0] = -(R + L) / (R - L);
        m_uiTransform.translate[1] = -(T + B) / (T - B);

        void* pointer = m_uniformBuffer->map();
        memcpy(pointer, &m_uiTransform, sizeof(UITransform));
    }

    // update draw buffer
    {
        ImDrawData* imDrawData = ImGui::GetDrawData();
        uint32_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
        uint32_t indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

        if ((vertexBufferSize == 0) || (indexBufferSize == 0))
        {
            return;
        }

        // Update buffers only if vertex or index count has been changed compared to current buffer size

        // Vertex buffer
        if ((m_vertexBuffer == nullptr) || (m_vertexBuffer->getSize() != vertexBufferSize))
        {
            BufferDescriptor descriptor{};
            descriptor.size = vertexBufferSize;
            descriptor.usage = BufferUsageFlagBits::kVertex;

            m_vertexBuffer = m_device->createBuffer(descriptor);
            m_vertexBuffer->map();
        }

        // Index buffer
        if ((m_indexBuffer == nullptr) || (m_indexBuffer->getSize() < indexBufferSize))
        {
            BufferDescriptor descriptor{};
            descriptor.size = indexBufferSize;
            descriptor.usage = BufferUsageFlagBits::kIndex;

            m_indexBuffer = m_device->createBuffer(descriptor);
            m_indexBuffer->map();
        }

        // Upload data
        ImDrawVert* vtxDst = (ImDrawVert*)m_vertexBuffer->map();
        ImDrawIdx* idxDst = (ImDrawIdx*)m_indexBuffer->map();

        for (int n = 0; n < imDrawData->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = imDrawData->CmdLists[n];
            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmd_list->VtxBuffer.Size;
            idxDst += cmd_list->IdxBuffer.Size;
        }

        // Flush if need
    }
}

void NativeImGui::draw(CommandEncoder* commandEncoder, TextureView* renderView)
{
    ImDrawData* imDrawData = ImGui::GetDrawData();

    if (imDrawData && imDrawData->CmdListsCount > 0)
    {
        ImGuiIO& io = ImGui::GetIO();

        ColorAttachment colorAttachment{
            .renderView = renderView
        };
        colorAttachment.loadOp = LoadOp::kLoad;
        colorAttachment.storeOp = StoreOp::kStore;

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { colorAttachment }
        };

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_pipeline.get());
        renderPassEncoder->setBindGroup(0, m_bindGroups[0].get());
        renderPassEncoder->setBindGroup(1, m_bindGroups[1].get());
        renderPassEncoder->setViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y, 0, 1);
        renderPassEncoder->setVertexBuffer(0, m_vertexBuffer.get());
        renderPassEncoder->setIndexBuffer(m_indexBuffer.get(), IndexFormat::kUint16);

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
                renderPassEncoder->setScissor(scissorX, scissorY, scissorWidth, scissorHeight);
                renderPassEncoder->drawIndexed(pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
                indexOffset += pcmd->ElemCount;
            }
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
        renderPassEncoder->end();
    }
}

void NativeImGui::clear()
{
    ImGui::DestroyContext();

    m_fontSampler.reset();
    m_fontTextureView.reset();
    m_fontTexture.reset();

    m_fontBuffer.reset();
    m_uniformBuffer.reset();
    m_vertexBuffer.reset();
    m_indexBuffer.reset();

    m_pipeline.reset();
    m_pipelineLayout.reset();
    m_bindGroups.clear();
    m_bindGroupLayouts.clear();
}

} // namespace jipu