#include "wgpu_triangle2.h"
#include "file.h"
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUTriangleSample2::WGPUTriangleSample2(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
{
}

WGPUTriangleSample2::~WGPUTriangleSample2()
{
    // TODO: check ways release and destory.

    wgpuRenderPipelineRelease(m_renderPipeline);
    wgpuPipelineLayoutRelease(m_pipelineLayout);
    wgpuShaderModuleRelease(m_vertSPIRVShaderModule);
    wgpuShaderModuleRelease(m_fragSPIRVShaderModule);
    // wgpuShaderModuleRelease(m_vertWGSLShaderModule);
    // wgpuShaderModuleRelease(m_fragWGSLShaderModule);

    wgpuQueueRelease(m_queue);
    wgpuDeviceDestroy(m_device);
    wgpuDeviceRelease(m_device);
    wgpuAdapterRelease(m_adapter);
    wgpuSurfaceRelease(m_surface);
    wgpuInstanceRelease(m_instance);
}

void WGPUTriangleSample2::init()
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

void WGPUTriangleSample2::update()
{
    WGPUSample::update();
}

void WGPUTriangleSample2::draw()
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

void WGPUTriangleSample2::createInstance()
{
    WGPUInstanceDescriptor descriptor{};
    descriptor.nextInChain = NULL;
    m_instance = wgpuCreateInstance({}); // TODO

    assert(m_instance);
}

void WGPUTriangleSample2::createSurface()
{
    WGPUChainedStruct chain{};
#if defined(__ANDROID__) || defined(ANDROID)
    chain.sType = WGPUSType_SurfaceSourceAndroidNativeWindow;

    WGPUSurfaceDescriptorFromAndroidNativeWindow surfaceDescriptor{};
    surfaceDescriptor.chain = chain;
    surfaceDescriptor.window = getWindowHandle();
#elif defined(__linux__)
    // TODO
#elif defined(__APPLE__)
    chain.sType = WGPUSType_SurfaceSourceMetalLayer;

    WGPUSurfaceDescriptorFromMetalLayer surfaceDescriptor{};
    surfaceDescriptor.chain = chain;
    surfaceDescriptor.layer = getWindowHandle();
#elif defined(WIN32)
    // TODO
#endif

    std::string label = "Surface";
    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct const*>(&surfaceDescriptor);
    surfaceDesc.label = WGPUStringView{ .data = label.data(), .length = label.length() };

    m_surface = wgpuInstanceCreateSurface(m_instance, &surfaceDesc);

    assert(m_surface);
}

void WGPUTriangleSample2::createAdapter()
{
    auto cb = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void* userdata) {
        if (status != WGPURequestAdapterStatus_Success)
        {
            throw std::runtime_error("Failed to request adapter.");
        }

        *static_cast<WGPUAdapter*>(userdata) = adapter;
    };

    WGPURequestAdapterOptions descriptor{
        .compatibleSurface = m_surface,
        .powerPreference = WGPUPowerPreference_HighPerformance,
#if defined(__ANDROID__) || defined(ANDROID)
        .backendType = WGPUBackendType_Vulkan,
#elif defined(__linux__)
        .backendType = WGPUBackendType_Vulkan,
#elif defined(__APPLE__)
        .backendType = WGPUBackendType_Vulkan,
#elif defined(WIN32)
        .backendType = WGPUBackendType_D3D12,
#endif
        .forceFallbackAdapter = false,
    };

    wgpuInstanceRequestAdapter(m_instance, &descriptor, cb, &m_adapter);

    assert(m_adapter);
}

void WGPUTriangleSample2::createDevice()
{
    auto cb = [](WGPURequestDeviceStatus status, WGPUDevice device, struct WGPUStringView message, void* userdata) {
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

void WGPUTriangleSample2::createSurfaceConfigure()
{
    auto status = wgpuSurfaceGetCapabilities(m_surface, m_adapter, &m_surfaceCapabilities);
    if (status != WGPUStatus_Success)
        throw std::runtime_error("Failed to get surface capabilities.");

    WGPUTextureFormat format = WGPUTextureFormat_Undefined;
    for (auto i = 0; i < m_surfaceCapabilities.formatCount; ++i)
    {
        // TODO
        format = m_surfaceCapabilities.formats[0];
    }

    WGPUCompositeAlphaMode alphaMode = WGPUCompositeAlphaMode_Auto;
    for (auto i = 0; i < m_surfaceCapabilities.alphaModeCount; ++i)
    {
        // TODO
        alphaMode = m_surfaceCapabilities.alphaModes[0];
    }

    WGPUPresentMode presentMode = WGPUPresentMode_Fifo;
    for (auto i = 0; i < m_surfaceCapabilities.presentModeCount; ++i)
    {
        // TODO
        // presentMode = m_surfaceCapabilities.presentModes[0];
    }

    WGPUTextureUsage usage = WGPUTextureUsage_None;
    if ((m_surfaceCapabilities.usages & WGPUTextureUsage_RenderAttachment) != 0)
    {
        usage = WGPUTextureUsage_RenderAttachment;
    }

    m_surfaceConfigure = WGPUSurfaceConfiguration{
        .device = m_device,
        .format = format,
        .usage = usage,
        .alphaMode = alphaMode,
        .width = m_width,
        .height = m_height,
        .presentMode = presentMode,
    };

    wgpuSurfaceConfigure(m_surface, &m_surfaceConfigure);
}

void WGPUTriangleSample2::createQueue()
{
    m_queue = wgpuDeviceGetQueue(m_device);

    assert(m_queue);
}

void WGPUTriangleSample2::createShaderModule()
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

        m_vertSPIRVShaderModule = wgpuDeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

        WGPUShaderModuleSPIRVDescriptor fragShaderModuleSPIRVDescriptor{};
        fragShaderModuleSPIRVDescriptor.chain.sType = WGPUSType_ShaderSourceSPIRV;
        fragShaderModuleSPIRVDescriptor.code = reinterpret_cast<const uint32_t*>(fragShaderSource.data());
        fragShaderModuleSPIRVDescriptor.codeSize = fragShaderSource.size();

        WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
        fragShaderModuleDescriptor.nextInChain = &fragShaderModuleSPIRVDescriptor.chain;

        m_fragSPIRVShaderModule = wgpuDeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);
    }

    // wgsl
    // {
    //     const char* vertexShaderCode = R"(
    //     @vertex
    //     fn main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
    //         let x = f32(i32(in_vertex_index) - 1);
    //         let y = f32(i32(in_vertex_index & 1u) * 2 - 1);
    //         return vec4<f32>(x, y, 0.0, 1.0);
    //     }
    // )";

    //     const char* fragmentShaderCode = R"(
    //     @fragment
    //     fn main() -> @location(0) vec4<f32> {
    //         return vec4<f32>(1.0, 0.0, 0.0, 1.0);
    //     }
    // )";

    //     WGPUShaderModuleWGSLDescriptor vertexShaderModuleWGSLDescriptor{};
    //     vertexShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    //     vertexShaderModuleWGSLDescriptor.code = vertexShaderCode;

    //     WGPUShaderModuleDescriptor vertexShaderModuleDescriptor{};
    //     vertexShaderModuleDescriptor.nextInChain = &vertexShaderModuleWGSLDescriptor.chain;

    //     m_vertWGSLShaderModule = wgpuDeviceCreateShaderModule(m_device, &vertexShaderModuleDescriptor);

    //     WGPUShaderModuleWGSLDescriptor fragShaderModuleWGSLDescriptor{};
    //     fragShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    //     fragShaderModuleWGSLDescriptor.code = fragmentShaderCode;

    //     WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
    //     fragShaderModuleDescriptor.nextInChain = &fragShaderModuleWGSLDescriptor.chain;

    //     m_fragWGSLShaderModule = wgpuDeviceCreateShaderModule(m_device, &fragShaderModuleDescriptor);
    // }
}

void WGPUTriangleSample2::createPipelineLayout()
{
    WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
    m_pipelineLayout = wgpuDeviceCreatePipelineLayout(m_device, &pipelineLayoutDescriptor);

    assert(m_pipelineLayout);
}

void WGPUTriangleSample2::createPipeline()
{
    WGPUPrimitiveState primitiveState{};
    primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode = WGPUCullMode_None;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

    std::string entryPoint = "main";
    WGPUVertexState vertexState{};
    vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.length() };
    vertexState.module = m_vertSPIRVShaderModule;

    WGPUColorTargetState colorTargetState{};
    colorTargetState.format = m_surfaceCapabilities.formats[0];
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragState{};
    fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.length() };
    fragState.module = m_fragSPIRVShaderModule;
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

    assert(m_renderPipeline);
}

} // namespace jipu