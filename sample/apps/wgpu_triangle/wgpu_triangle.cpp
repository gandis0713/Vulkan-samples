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
    // TODO: check ways release and destory.
    wgpuRenderPipelineRelease(m_renderPipeline);
    wgpuPipelineLayoutRelease(m_pipelineLayout);
    wgpuShaderModuleRelease(m_vertSPIRVShaderModule);
    wgpuShaderModuleRelease(m_fragSPIRVShaderModule);
    wgpuShaderModuleRelease(m_vertWGSLShaderModule);
    wgpuShaderModuleRelease(m_fragWGSLShaderModule);

    wgpuQueueRelease(m_queue);
    wgpuDeviceDestroy(m_device);
    wgpuDeviceRelease(m_device);
    wgpuAdapterRelease(m_adapter);
    wgpuInstanceRelease(m_instance);
}

void WGPUTriangleSample::init()
{
    WGPUSample::init();

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

void WGPUTriangleSample::update()
{
    WGPUSample::update();
}

void WGPUTriangleSample::draw()
{
    WGPUSurfaceTexture surfaceTexture{};
    wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpuTextureCreateView(surfaceTexture.texture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(m_device, &commandEncoderDescriptor);

    WGPURenderPassColorAttachment colorAttachment{};
    colorAttachment.view = surfaceTextureView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.clearValue = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };

    WGPURenderPassDescriptor renderPassDescriptor{};
    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &colorAttachment;

    WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

    wgpuRenderPassEncoderSetPipeline(renderPassEncoder, m_renderPipeline);
    wgpuRenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);
    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpuQueueSubmit(m_queue, 1, &commandBuffer);
    wgpuSurfacePresent(m_surface);

    wgpuCommandBufferRelease(commandBuffer);
    wgpuCommandEncoderRelease(commandEncoder);
    wgpuTextureViewRelease(surfaceTextureView);
    wgpuTextureRelease(surfaceTexture.texture);
}

void WGPUTriangleSample::createInstance()
{
    WGPUInstanceDescriptor descriptor{};
    descriptor.nextInChain = NULL;
    m_instance = wgpuCreateInstance({}); // TODO

    assert(m_instance);
}

void WGPUTriangleSample::createSurface()
{
    WGPUChainedStruct chain{};
    chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;

    WGPUSurfaceDescriptorFromMetalLayer metalLayerDesc = {};
    metalLayerDesc.chain = chain;
    metalLayerDesc.layer = getWindowHandle();

    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct const*>(&metalLayerDesc);
    surfaceDesc.label = "Metal Surface";

    m_surface = wgpuInstanceCreateSurface(m_instance, &surfaceDesc);

    assert(m_surface);
}

void WGPUTriangleSample::createAdapter()
{
    auto cb = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, WGPU_NULLABLE void* userdata) {
        if (status != WGPURequestAdapterStatus_Success)
        {
            throw std::runtime_error("Failed to request adapter.");
        }

        *static_cast<WGPUAdapter*>(userdata) = adapter;
    };

    WGPURequestAdapterOptions descriptor{
        .backendType = WGPUBackendType_Metal,
        .compatibleSurface = m_surface,
        .forceFallbackAdapter = false,
        .powerPreference = WGPUPowerPreference_HighPerformance
    };
    wgpuInstanceRequestAdapter(m_instance, &descriptor, cb, &m_adapter);

    assert(m_adapter);
}

void WGPUTriangleSample::createDevice()
{
    auto cb = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, WGPU_NULLABLE void* userdata) {
        if (status != WGPURequestDeviceStatus_Success)
        {
            throw std::runtime_error("Failed to request device.");
        }

        *static_cast<WGPUDevice*>(userdata) = device;
    };

    WGPUDeviceDescriptor deviceDescriptor{};
    wgpuAdapterRequestDevice(m_adapter, &deviceDescriptor, cb, &m_device);

    assert(m_device);
}

void WGPUTriangleSample::createSurfaceConfigure()
{
    wgpuSurfaceGetCapabilities(m_surface, m_adapter, &m_surfaceCapabilities);

    m_surfaceConfigure = {
        .device = m_device,
        .usage = WGPUTextureUsage_RenderAttachment,
        .format = m_surfaceCapabilities.formats[0],
        .presentMode = WGPUPresentMode_Fifo,
        .alphaMode = m_surfaceCapabilities.alphaModes[0],
        .width = m_width,
        .height = m_height,
    };

    wgpuSurfaceConfigure(m_surface, &m_surfaceConfigure);
}

void WGPUTriangleSample::createQueue()
{
    m_queue = wgpuDeviceGetQueue(m_device);

    assert(m_queue);
}

void WGPUTriangleSample::createShaderModule()
{
    // spriv
    {
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "triangle.vert.spv", m_handle);
        std::vector<char> fragShaderSource = utils::readFile(m_appDir / "triangle.frag.spv", m_handle);

        WGPUShaderModuleSPIRVDescriptor vertexShaderModuleSPIRVDescriptor{};
        vertexShaderModuleSPIRVDescriptor.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
        vertexShaderModuleSPIRVDescriptor.code = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());
        vertexShaderModuleSPIRVDescriptor.codeSize = vertexShaderSource.size();

        WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
        vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleSPIRVDescriptor.chain;

        m_vertSPIRVShaderModule = wgpuDeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

        WGPUShaderModuleSPIRVDescriptor fragShaderModuleSPIRVDescriptor{};
        fragShaderModuleSPIRVDescriptor.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
        fragShaderModuleSPIRVDescriptor.code = reinterpret_cast<const uint32_t*>(fragShaderSource.data());
        fragShaderModuleSPIRVDescriptor.codeSize = fragShaderSource.size();

        WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
        fragShaderModuleDescriptor.nextInChain = &fragShaderModuleSPIRVDescriptor.chain;

        m_fragSPIRVShaderModule = wgpuDeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);
    }

    {
        const char* vertexShaderCode = R"(
        @vertex
        fn main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
            let x = f32(i32(in_vertex_index) - 1);
            let y = f32(i32(in_vertex_index & 1u) * 2 - 1);
            return vec4<f32>(x, y, 0.0, 1.0);
        }
    )";

        const char* fragmentShaderCode = R"(
        @fragment
        fn main() -> @location(0) vec4<f32> {
            return vec4<f32>(1.0, 0.0, 0.0, 1.0);
        }
    )";

        WGPUShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
        vertexShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        vertexShaderModuleWGSLDescriptor.code = vertexShaderCode;

        WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
        vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleWGSLDescriptor.chain;

        m_vertWGSLShaderModule = wgpuDeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

        WGPUShaderModuleWGSLDescriptor fragShaderModuleWGSLDescriptor{};
        fragShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        fragShaderModuleWGSLDescriptor.code = fragmentShaderCode;

        WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
        fragShaderModuleDescriptor.nextInChain = &fragShaderModuleWGSLDescriptor.chain;

        m_fragWGSLShaderModule = wgpuDeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);
    }
}

void WGPUTriangleSample::createPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);
}

void WGPUTriangleSample::createPipeline()
{

    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_None;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    WGPUVertexState vertexState{};
    vertexState.entryPoint = "main";
    if (m_useSPIRV)
        vertexState.module = m_vertSPIRVShaderModule;
    else
        vertexState.module = m_vertWGSLShaderModule;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceCapabilities.formats[0];
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.entryPoint = "main";
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

    m_renderPipeline = wgpuDeviceCreateRenderPipeline(m_device, &renderPipelineDescriptor);
}

} // namespace jipu