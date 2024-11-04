#include "wgpu_triangle.h"

#include "file.h"
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUTriangleSample::WGPUTriangleSample(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
}

WGPUTriangleSample::~WGPUTriangleSample()
{
    finalizeContext();
}

void WGPUTriangleSample::init()
{
    WGPUSample::init();

    changeAPI(APIType::kJipu);
    // changeAPI(APIType::kDawn);
}

void WGPUTriangleSample::update()
{
    WGPUSample::update();
}

void WGPUTriangleSample::draw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpu().SurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu().TextureCreateView(surfaceTexture.texture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu().DeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

    WGPURenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;

    WGPURenderPassEncoder renderPassEncoder = wgpu().CommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpu().RenderPassEncoderSetPipeline(renderPassEncoder, m_renderPipeline);
    wgpu().RenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);
    wgpu().RenderPassEncoderEnd(renderPassEncoder);
    wgpu().RenderPassEncoderRelease(renderPassEncoder);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu().CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu().QueueSubmit(m_queue, 1, &commandBuffer);
    wgpu().SurfacePresent(m_surface);

    wgpu().CommandBufferRelease(commandBuffer);
    wgpu().CommandEncoderRelease(commandEncoder);
    wgpu().TextureViewRelease(surfaceTextureView);
    wgpu().TextureRelease(surfaceTexture.texture);
}

void WGPUTriangleSample::initializeContext()
{
    createInstance();
    createSurface();
    createAdapter();
    createDevice();
    createSurfaceConfigure();
    createQueue();
    createShaderModule();
    createPipelineLayout();
    createPipeline();
}

void WGPUTriangleSample::finalizeContext()
{
    // TODO: check ways release and destory.
    if (m_renderPipeline)
    {
        wgpu().RenderPipelineRelease(m_renderPipeline);
        m_renderPipeline = nullptr;
    }

    if (m_pipelineLayout)
    {
        wgpu().PipelineLayoutRelease(m_pipelineLayout);
        m_pipelineLayout = nullptr;
    }

    if (m_vertSPIRVShaderModule)
    {
        wgpu().ShaderModuleRelease(m_vertSPIRVShaderModule);
        m_vertSPIRVShaderModule = nullptr;
    }

    if (m_fragSPIRVShaderModule)
    {
        wgpu().ShaderModuleRelease(m_fragSPIRVShaderModule);
        m_fragSPIRVShaderModule = nullptr;
    }

    if (m_vertWGSLShaderModule)
    {
        wgpu().ShaderModuleRelease(m_vertWGSLShaderModule);
        m_vertWGSLShaderModule = nullptr;
    }

    if (m_fragWGSLShaderModule)
    {
        wgpu().ShaderModuleRelease(m_fragWGSLShaderModule);
        m_fragWGSLShaderModule = nullptr;
    }

    if (m_queue)
    {
        wgpu().QueueRelease(m_queue);
        m_queue = nullptr;
    }

    if (m_device)
    {
        wgpu().DeviceDestroy(m_device);
        wgpu().DeviceRelease(m_device);
        m_device = nullptr;
    }

    if (m_adapter)
    {
        wgpu().AdapterRelease(m_adapter);
        m_adapter = nullptr;
    }

    if (m_surface)
    {
        wgpu().SurfaceRelease(m_surface);
        m_surface = nullptr;
    }

    if (m_instance)
    {
        wgpu().InstanceRelease(m_instance);
        m_instance = nullptr;
    }
}

void WGPUTriangleSample::createShaderModule()
{
    // spriv
    {
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "triangle.vert.spv", m_handle);
        std::vector<char> fragShaderSource = utils::readFile(m_appDir / "triangle.frag.spv", m_handle);

        WGPUShaderModuleSPIRVDescriptor vertexShaderModuleSPIRVDescriptor{};
        vertexShaderModuleSPIRVDescriptor.chain.sType = WGPUSType_ShaderSourceSPIRV;
        vertexShaderModuleSPIRVDescriptor.code = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());
        vertexShaderModuleSPIRVDescriptor.codeSize = vertexShaderSource.size();

        WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
        vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleSPIRVDescriptor.chain;

        m_vertSPIRVShaderModule = wgpu().DeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

        assert(m_vertSPIRVShaderModule);

        WGPUShaderModuleSPIRVDescriptor fragShaderModuleSPIRVDescriptor{};
        fragShaderModuleSPIRVDescriptor.chain.sType = WGPUSType_ShaderSourceSPIRV;
        fragShaderModuleSPIRVDescriptor.code = reinterpret_cast<const uint32_t*>(fragShaderSource.data());
        fragShaderModuleSPIRVDescriptor.codeSize = fragShaderSource.size();

        WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
        fragShaderModuleDescriptor.nextInChain = &fragShaderModuleSPIRVDescriptor.chain;

        m_fragSPIRVShaderModule = wgpu().DeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);

        assert(m_fragSPIRVShaderModule);
    }

    {
        std::string vertexShaderCode = R"(
        @vertex
        fn main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
            let x = f32(i32(in_vertex_index) - 1);
            let y = f32(i32(in_vertex_index & 1u) * 2 - 1);
            return vec4<f32>(x, y, 0.0, 1.0);
        }
    )";

        std::string fragmentShaderCode = R"(
        @fragment
        fn main() -> @location(0) vec4<f32> {
            return vec4<f32>(1.0, 0.0, 0.0, 1.0);
        }
    )";

        WGPUShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
        vertexShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        vertexShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = vertexShaderCode.data(), .length = vertexShaderCode.size() };

        WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
        vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleWGSLDescriptor.chain;

        m_vertWGSLShaderModule = wgpu().DeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

        assert(m_vertWGSLShaderModule);

        WGPUShaderModuleWGSLDescriptor fragShaderModuleWGSLDescriptor{};
        fragShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
        fragShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = fragmentShaderCode.data(), .length = fragmentShaderCode.size() };

        WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
        fragShaderModuleDescriptor.nextInChain = &fragShaderModuleWGSLDescriptor.chain;

        m_fragWGSLShaderModule = wgpu().DeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);

        assert(m_fragWGSLShaderModule);
    }
}

void WGPUTriangleSample::createPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_pipelineLayout = wgpu().DeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_pipelineLayout);
}

void WGPUTriangleSample::createPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_None;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    std::string entryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    if (m_useSPIRV)
        vertexState.module = m_vertSPIRVShaderModule;
    else
        vertexState.module = m_vertWGSLShaderModule;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceCapabilities.formats[0];
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
    if (m_useSPIRV)
        fragState.module = m_fragSPIRVShaderModule;
    else
        fragState.module = m_fragWGSLShaderModule;

    fragState.targetCount = 1;
    fragState.targets = &colorTargetState;

    // WGPUDepthStencilState depthStencilState{};
    // depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;

    WGPUMultisampleState multisampleState{};
    multisampleState.count = 1;
    multisampleState.mask = 0xFFFFFFFF;

    WGPURenderPipelineDescriptor renderPipelineDescriptor{};
    renderPipelineDescriptor.layout = m_pipelineLayout;
    renderPipelineDescriptor.primitive = primitiveState;
    renderPipelineDescriptor.multisample = multisampleState;
    renderPipelineDescriptor.vertex = vertexState;
    renderPipelineDescriptor.fragment = &fragState;

    m_renderPipeline = wgpu().DeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);

    assert(m_renderPipeline);
}

} // namespace jipu