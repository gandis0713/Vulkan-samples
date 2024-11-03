#include "wgpu_triangle2.h"

#include "file.h"
#include <spdlog/spdlog.h>

namespace jipu
{

WGPUTriangleSample2::WGPUTriangleSample2(const WGPUSampleDescriptor& descriptor)
    : WGPUSample(descriptor)
    , m_wgpuContexts{ { WGPUSample::LibType::kDawn, WGPUContext{} }, { WGPUSample::LibType::kJipu, WGPUContext{} } }
{
}

WGPUTriangleSample2::~WGPUTriangleSample2()
{
    // TODO: check ways release and destory.
    // wgpuRenderPipelineRelease(m_renderPipeline);
    // wgpuPipelineLayoutRelease(m_pipelineLayout);
    // wgpuShaderModuleRelease(m_vertSPIRVShaderModule);
    // wgpuShaderModuleRelease(m_fragSPIRVShaderModule);
    // wgpuShaderModuleRelease(m_vertWGSLShaderModule);
    // wgpuShaderModuleRelease(m_fragWGSLShaderModule);

    // wgpuQueueRelease(m_queue);
    // wgpuDeviceDestroy(m_device);
    // wgpuDeviceRelease(m_device);
    // wgpuAdapterRelease(m_adapter);
    // wgpuSurfaceRelease(m_surface);
    // wgpuInstanceRelease(m_instance);
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
    wgpu().SurfaceGetCurrentTexture(getContext().surface, &surfaceTexture);

    WGPUTextureView surfaceTextureView = wgpu().TextureCreateView(surfaceTexture.texture, NULL);

    WGPUCommandEncoderDescriptor commandEncoderDescriptor{};
    WGPUCommandEncoder commandEncoder = wgpu().DeviceCreateCommandEncoder(getContext().device, &commandEncoderDescriptor);

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

    wgpu().RenderPassEncoderSetPipeline(renderPassEncoder, getContext().renderPipeline);
    wgpu().RenderPassEncoderDraw(renderPassEncoder, 3, 1, 0, 0);
    wgpu().RenderPassEncoderEnd(renderPassEncoder);
    wgpu().RenderPassEncoderRelease(renderPassEncoder);

    WGPUCommandBufferDescriptor commandBufferDescriptor{};
    WGPUCommandBuffer commandBuffer = wgpu().CommandEncoderFinish(commandEncoder, &commandBufferDescriptor);

    wgpu().QueueSubmit(getContext().queue, 1, &commandBuffer);
    wgpu().SurfacePresent(getContext().surface);

    wgpu().CommandBufferRelease(commandBuffer);
    wgpu().CommandEncoderRelease(commandEncoder);
    wgpu().TextureViewRelease(surfaceTextureView);
    wgpu().TextureRelease(surfaceTexture.texture);
}

void WGPUTriangleSample2::createInstance()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

        WGPUInstanceDescriptor descriptor{};
        descriptor.nextInChain = NULL;

        m_wgpuContexts[type].instance = wgpu(type).CreateInstance({}); // TODO: use descriptor
        assert(m_wgpuContexts[type].instance);
    }
}

void WGPUTriangleSample2::createSurface()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

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
        surfaceDesc.label = WGPUStringView{ .data = label.data(), .length = label.size() };

        m_wgpuContexts[type].surface = wgpu(type).InstanceCreateSurface(m_wgpuContexts[type].instance, &surfaceDesc);
        assert(m_wgpuContexts[type].surface);
    }
}

void WGPUTriangleSample2::createAdapter()
{

    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

        auto cb = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void* userdata) {
            if (status != WGPURequestAdapterStatus_Success)
            {
                throw std::runtime_error("Failed to request adapter.");
            }

            *static_cast<WGPUAdapter*>(userdata) = adapter;
        };

        WGPURequestAdapterOptions descriptor{
            .compatibleSurface = m_wgpuContexts[type].surface,
            .powerPreference = WGPUPowerPreference_HighPerformance,
#if defined(__ANDROID__) || defined(ANDROID)
            .backendType = WGPUBackendType_Vulkan,
#elif defined(__linux__)
            .backendType = WGPUBackendType_Vulkan,
#elif defined(__APPLE__)
            .backendType = type == LibType::kDawn ? WGPUBackendType_Metal : WGPUBackendType_Vulkan,
#elif defined(WIN32)
            .backendType = WGPUBackendType_D3D12,
#endif
            .forceFallbackAdapter = false,
        };

        wgpu(type).InstanceRequestAdapter(m_wgpuContexts[type].instance, &descriptor, cb, &m_wgpuContexts[type].adapter);

        assert(m_wgpuContexts[type].adapter);
    }
}

void WGPUTriangleSample2::createDevice()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

        auto cb = [](WGPURequestDeviceStatus status, WGPUDevice device, struct WGPUStringView message, void* userdata) {
            if (status != WGPURequestDeviceStatus_Success)
            {
                throw std::runtime_error("Failed to request device.");
            }

            *static_cast<WGPUDevice*>(userdata) = device;
        };

        WGPUDeviceDescriptor deviceDescriptor{};

        wgpu(type).AdapterRequestDevice(m_wgpuContexts[type].adapter, &deviceDescriptor, cb, &m_wgpuContexts[type].device);

        assert(m_wgpuContexts[type].device);
    }
}

void WGPUTriangleSample2::createSurfaceConfigure()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

        auto status = wgpu(type).SurfaceGetCapabilities(m_wgpuContexts[type].surface, m_wgpuContexts[type].adapter, &m_wgpuContexts[type].surfaceCapabilities);
        if (status != WGPUStatus_Success)
            throw std::runtime_error("Failed to get surface capabilities.");

        auto& surfaceCapabilities = m_wgpuContexts[type].surfaceCapabilities;

        WGPUTextureFormat format = WGPUTextureFormat_Undefined;
        for (auto i = 0; i < surfaceCapabilities.formatCount; ++i)
        {
            // TODO
            format = surfaceCapabilities.formats[0];
        }

        WGPUCompositeAlphaMode alphaMode = WGPUCompositeAlphaMode_Auto;
        for (auto i = 0; i < surfaceCapabilities.alphaModeCount; ++i)
        {
            // TODO
            alphaMode = surfaceCapabilities.alphaModes[0];
        }

        WGPUPresentMode presentMode = WGPUPresentMode_Fifo;
        for (auto i = 0; i < surfaceCapabilities.presentModeCount; ++i)
        {
            // TODO
            // presentMode = surfaceCapabilities.presentModes[0];
        }

        WGPUTextureUsage usage = WGPUTextureUsage_None;
        if ((surfaceCapabilities.usages & WGPUTextureUsage_RenderAttachment) != 0)
        {
            usage = WGPUTextureUsage_RenderAttachment;
        }

        m_wgpuContexts[type].surfaceConfigure = WGPUSurfaceConfiguration{
            .device = m_wgpuContexts[type].device,
            .format = format,
            .usage = usage,
            .alphaMode = alphaMode,
            .width = m_width,
            .height = m_height,
            .presentMode = presentMode,
        };

        wgpu(type).SurfaceConfigure(m_wgpuContexts[type].surface, &m_wgpuContexts[type].surfaceConfigure);
    }
}

void WGPUTriangleSample2::createQueue()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

        m_wgpuContexts[type].queue = wgpu(type).DeviceGetQueue(m_wgpuContexts[type].device);

        assert(m_wgpuContexts[type].queue);
    }
}

void WGPUTriangleSample2::createShaderModule()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);
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

            m_wgpuContexts[type].vertSPIRVShaderModule = wgpu(type).DeviceCreateShaderModule(m_wgpuContexts[type].device, &vertexShaderModuleDescriptor);

            assert(m_wgpuContexts[type].vertSPIRVShaderModule);

            WGPUShaderModuleSPIRVDescriptor fragShaderModuleSPIRVDescriptor{};
            fragShaderModuleSPIRVDescriptor.chain.sType = WGPUSType_ShaderSourceSPIRV;
            fragShaderModuleSPIRVDescriptor.code = reinterpret_cast<const uint32_t*>(fragShaderSource.data());
            fragShaderModuleSPIRVDescriptor.codeSize = fragShaderSource.size();

            WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
            fragShaderModuleDescriptor.nextInChain = &fragShaderModuleSPIRVDescriptor.chain;

            m_wgpuContexts[type].fragSPIRVShaderModule = wgpu(type).DeviceCreateShaderModule(m_wgpuContexts[type].device, &fragShaderModuleDescriptor);

            assert(m_wgpuContexts[type].fragSPIRVShaderModule);
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

            m_wgpuContexts[type].vertWGSLShaderModule = wgpu(type).DeviceCreateShaderModule(m_wgpuContexts[type].device, &vertexShaderModuleDescriptor);

            assert(m_wgpuContexts[type].vertWGSLShaderModule);

            WGPUShaderModuleWGSLDescriptor fragShaderModuleWGSLDescriptor{};
            fragShaderModuleWGSLDescriptor.chain.sType = WGPUSType_ShaderSourceWGSL;
            fragShaderModuleWGSLDescriptor.code = WGPUStringView{ .data = fragmentShaderCode.data(), .length = fragmentShaderCode.size() };

            WGPUShaderModuleDescriptor fragShaderModuleDescriptor{};
            fragShaderModuleDescriptor.nextInChain = &fragShaderModuleWGSLDescriptor.chain;

            m_wgpuContexts[type].fragWGSLShaderModule = wgpu(type).DeviceCreateShaderModule(m_wgpuContexts[type].device, &fragShaderModuleDescriptor);

            assert(m_wgpuContexts[type].fragWGSLShaderModule);
        }
    }
}

void WGPUTriangleSample2::createPipelineLayout()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

        WGPUPipelineLayoutDescriptor pipelineLayoutDescriptor{};
        m_wgpuContexts[type].pipelineLayout = wgpu(type).DeviceCreatePipelineLayout(m_wgpuContexts[type].device, &pipelineLayoutDescriptor);

        assert(m_wgpuContexts[type].pipelineLayout);
    }
}

void WGPUTriangleSample2::createPipeline()
{
    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

        WGPUPrimitiveState primitiveState{};
        primitiveState.topology = WGPUPrimitiveTopology_TriangleList;
        primitiveState.cullMode = WGPUCullMode_None;
        primitiveState.frontFace = WGPUFrontFace_CCW;
        // primitiveState.stripIndexFormat = WGPUIndexFormat_Undefined;

        std::string entryPoint = "main";
        WGPUVertexState vertexState{};
        vertexState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
        if (m_useSPIRV)
            vertexState.module = m_wgpuContexts[type].vertSPIRVShaderModule;
        else
            vertexState.module = m_wgpuContexts[type].vertWGSLShaderModule;

        WGPUColorTargetState colorTargetState{};
        colorTargetState.format = m_wgpuContexts[type].surfaceCapabilities.formats[0];
        colorTargetState.writeMask = WGPUColorWriteMask_All;

        WGPUFragmentState fragState{};
        fragState.entryPoint = WGPUStringView{ .data = entryPoint.data(), .length = entryPoint.size() };
        if (m_useSPIRV)
            fragState.module = m_wgpuContexts[type].fragSPIRVShaderModule;
        else
            fragState.module = m_wgpuContexts[type].fragWGSLShaderModule;

        fragState.targetCount = 1;
        fragState.targets = &colorTargetState;

        // WGPUDepthStencilState depthStencilState{};
        // depthStencilState.format = WGPUTextureFormat_Depth24PlusStencil8;

        WGPUMultisampleState multisampleState{};
        multisampleState.count = 1;
        multisampleState.mask = 0xFFFFFFFF;

        WGPURenderPipelineDescriptor renderPipelineDescriptor{};
        renderPipelineDescriptor.layout = m_wgpuContexts[type].pipelineLayout;
        renderPipelineDescriptor.primitive = primitiveState;
        renderPipelineDescriptor.multisample = multisampleState;
        renderPipelineDescriptor.vertex = vertexState;
        renderPipelineDescriptor.fragment = &fragState;

        m_wgpuContexts[type].renderPipeline = wgpu(type).DeviceCreateRenderPipeline(m_wgpuContexts[type].device, &renderPipelineDescriptor);

        assert(m_wgpuContexts[type].renderPipeline);
    }
}

WGPUTriangleSample2::WGPUContext& WGPUTriangleSample2::getContext()
{
    LibType type = getLibType();
    return m_wgpuContexts.at(type);
}

} // namespace jipu