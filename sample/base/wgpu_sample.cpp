#include "wgpu_sample.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

WGPUSample::WGPUSample(const WGPUSampleDescriptor& descriptor)
    : Window(descriptor.windowDescriptor)
    , m_appPath(descriptor.path)
    , m_appDir(descriptor.path.parent_path())
{
#if defined(__ANDROID__) || defined(ANDROID)
    std::unordered_map<APIType, std::string> wgpuLibNames{
        { APIType::kJipu, "libjipu.so" },
        { APIType::kDawn, "libwebgpu_dawn.so" },
    };
#elif defined(__linux__)
    std::unordered_map<APIType, std::string> wgpuLibNames{
        { APIType::kJipu, "libjipu.so" },
        { APIType::kDawn, "libwebgpu_dawn.so" },
    };
#elif defined(__APPLE__)
    std::unordered_map<APIType, std::string> wgpuLibNames{
        { APIType::kJipu, "libjipu.dylib" },
        { APIType::kDawn, "libwebgpu_dawn.dylib" },
    };
#elif defined(WIN32)
    std::unordered_map<APIType, std::string> wgpuLibNames{
        { APIType::kJipu, "jipu.dll" },
        { APIType::kDawn, "webgpu_dawn.dll" },
    };
#endif

    for (auto i = 0; i < static_cast<uint16_t>(APIType::kCount); ++i)
    {
        auto type = static_cast<APIType>(i);

        m_wgpuLibs.insert({ type, DyLib{} });
        if (!m_wgpuLibs[type].open(wgpuLibNames[type].c_str()))
        {
            throw std::runtime_error(fmt::format("Failed to open library: {}", wgpuLibNames[type].c_str()));
        }

        m_wgpuAPIs.insert({ type, WebGPUAPI{} });
        if (!m_wgpuAPIs[type].loadProcs(&m_wgpuLibs[type]))
        {
            throw std::runtime_error(fmt::format("Failed to load procs: {}", wgpuLibNames[type].c_str()));
        }
    }
}

WGPUSample::~WGPUSample()
{
}

void WGPUSample::init()
{
    Window::init();
}

void WGPUSample::onBeforeUpdate()
{
    m_fps.update();

    recordImGui({ [&]() {
        windowImGui(
            "Common", { [&]() {
                           ImGui::Separator();
                           //    ImGui::Text("API Type");
                           ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "API Type");
                           if (ImGui::RadioButton("Jipu", m_apiType == APIType::kJipu))
                           {
                               m_changeAPIType = APIType::kJipu;
                           }
                           else if (ImGui::RadioButton("Dawn", m_apiType == APIType::kDawn))
                           {
                               m_changeAPIType = APIType::kDawn;
                           }
                           ImGui::Separator();
                       },
                        [&]() {
                            ImGui::Separator();
                            // ImGui::Text("Profiling");
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Profiling");
                            drawPolyline("FPS", m_fps.getAll());
                            ImGui::Separator();
                        } });
    } });
}

void WGPUSample::onUpdate()
{
    changeAPI(m_changeAPIType);
}

void WGPUSample::onAfterUpdate()
{
    buildImGui();
}

void WGPUSample::onResize(uint32_t width, uint32_t height)
{
    createSurfaceConfigure();
    if (m_imgui.has_value())
    {
        m_imgui.value().resize();
    }
}

void WGPUSample::recordImGui(std::vector<std::function<void()>> cmds)
{
    if (m_imgui.has_value())
    {
        m_imgui.value().record(cmds);
    }
}

void WGPUSample::buildImGui()
{
    if (m_imgui.has_value())
    {
        m_imgui.value().build();
    }
}

void WGPUSample::windowImGui(const char* title, std::vector<std::function<void()>> uis)
{
    if (m_imgui.has_value())
    {
        // set display size and mouse state.
        {
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2((float)m_width, (float)m_height);
            io.MousePos = ImVec2(m_mouseX, m_mouseY);
            io.MouseDown[0] = m_leftMouseButton;
            io.MouseDown[1] = m_rightMouseButton;
            io.MouseDown[2] = m_middleMouseButton;
        }

        m_imgui.value().window(title, uis);
    }
}
void WGPUSample::drawImGui(WGPUCommandEncoder commandEncoder, WGPUTextureView renderView)
{
    if (m_imgui.has_value())
    {
        m_imgui.value().draw(commandEncoder, renderView);
    }
}

void WGPUSample::initializeContext()
{
    createInstance();
    createSurface();
    createAdapter();
    createDevice();
    createSurfaceConfigure();
    createQueue();

    if (m_imgui.has_value())
    {
        m_imgui.value().initialize();
    }
}

void WGPUSample::finalizeContext()
{
    if (m_imgui.has_value())
        m_imgui.value().finalize();

    if (m_queue)
    {
        wgpu.QueueRelease(m_queue);
        m_queue = nullptr;
    }

    if (m_device)
    {
        wgpu.DeviceDestroy(m_device);
        wgpu.DeviceRelease(m_device);
        m_device = nullptr;
    }

    if (m_adapter)
    {
        wgpu.AdapterRelease(m_adapter);
        m_adapter = nullptr;
    }

    if (m_surface)
    {
        wgpu.SurfaceRelease(m_surface);
        m_surface = nullptr;
    }

    if (m_instance)
    {
        wgpu.InstanceRelease(m_instance);
        m_instance = nullptr;
    }
}

void WGPUSample::changeAPI(WGPUSample::APIType type)
{
    if (type == APIType::kUndefined)
        return;

    m_changeAPIType = type;
    if (m_apiType == m_changeAPIType)
        return;

    m_fps.clear();

    finalizeContext();

    m_apiType = type;
    wgpu = m_wgpuAPIs[m_apiType];

    initializeContext();
}

WGPUSample::APIType WGPUSample::getAPIType()
{
    return m_apiType;
}

void WGPUSample::createInstance()
{
    WGPUInstanceDescriptor descriptor{};
    descriptor.nextInChain = NULL;

    m_instance = wgpu.CreateInstance({}); // TODO: use descriptor
    assert(m_instance);
}

void WGPUSample::createSurface()
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
    surfaceDesc.label = WGPUStringView{ .data = label.data(), .length = label.size() };

    m_surface = wgpu.InstanceCreateSurface(m_instance, &surfaceDesc);
    assert(m_surface);
}

void WGPUSample::createAdapter()
{
    WGPURequestAdapterCallbackInfo2 callbackInfo{ WGPU_REQUEST_ADAPTER_CALLBACK_INFO_2_INIT };
    callbackInfo.callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
        if (status != WGPURequestAdapterStatus_Success)
        {
            throw std::runtime_error("Failed to request adapter.");
        }

        *static_cast<WGPUAdapter*>(userdata1) = adapter;
    };
    callbackInfo.userdata1 = &m_adapter;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;

    WGPURequestAdapterOptions descriptor{
        .compatibleSurface = m_surface,
        .powerPreference = WGPUPowerPreference_HighPerformance,
#if defined(__ANDROID__) || defined(ANDROID)
        .backendType = WGPUBackendType_Vulkan,
#elif defined(__linux__)
        .backendType = WGPUBackendType_Vulkan,
#elif defined(__APPLE__)
        .backendType = getAPIType() == APIType::kDawn ? WGPUBackendType_Metal : WGPUBackendType_Vulkan,
#elif defined(WIN32)
        .backendType = WGPUBackendType_D3D12,
#endif
        .forceFallbackAdapter = false,
    };

    auto future = wgpu.InstanceRequestAdapter2(m_instance, &descriptor, callbackInfo);

    WGPUFutureWaitInfo waitInfo{ .future = future, .completed = false };
    wgpu.InstanceWaitAny(m_instance, 1, &waitInfo, 0);

    assert(m_adapter);
}

void WGPUSample::createDevice()
{
    WGPUUncapturedErrorCallbackInfo2 errorCallbackInfo{};
    errorCallbackInfo.callback = [](WGPUDevice const* device, WGPUErrorType type, WGPUStringView message, void* userdata1, void* userdata2) {
        std::string msg(message.data, message.length);

        switch (type)
        {
        case WGPUErrorType_Validation:
            spdlog::error("Validation error: {}", msg.data());
            break;
        case WGPUErrorType_OutOfMemory:
            spdlog::error("Out of memory error: {}", msg.data());
            break;
        case WGPUErrorType_Unknown:
            spdlog::error("Unknown error: {}", msg.data());
            break;
        case WGPUErrorType_DeviceLost:
            spdlog::error("Device lost error: {}", msg.data());
            break;
        case WGPUErrorType_Internal:
            spdlog::error("Internal error: {}", msg.data());
            break;
        case WGPUErrorType_NoError:
        default:
            spdlog::error("No error: {}", msg.data());
            break;
        }
    };

    WGPUDeviceDescriptor deviceDescriptor{};
    deviceDescriptor.uncapturedErrorCallbackInfo2 = errorCallbackInfo;

    WGPURequestDeviceCallbackInfo2 callbackInfo{};
    callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    // callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
    // callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.userdata1 = &m_device;
    callbackInfo.callback = [](WGPURequestDeviceStatus status, WGPUDevice device, struct WGPUStringView message, void* userdata1, void* userdata2) {
        if (status != WGPURequestDeviceStatus_Success)
        {
            throw std::runtime_error("Failed to request device.");
        }

        *static_cast<WGPUDevice*>(userdata1) = device;
    };

    auto future = wgpu.AdapterRequestDevice2(m_adapter, &deviceDescriptor, callbackInfo);

    // WGPUFutureWaitInfo waitInfo{ .future = future, .completed = false };
    // wgpu.InstanceWaitAny(m_instance, 1, &waitInfo, 0);

    // wgpu.InstanceProcessEvents(m_instance);

    assert(m_device);
}

void WGPUSample::createSurfaceConfigure()
{
    auto status = wgpu.SurfaceGetCapabilities(m_surface, m_adapter, &m_surfaceCapabilities);
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

    wgpu.SurfaceConfigure(m_surface, &m_surfaceConfigure);
}

void WGPUSample::createQueue()
{
    m_queue = wgpu.DeviceGetQueue(m_device);

    assert(m_queue);
}

void WGPUSample::drawPolyline(std::string title, std::deque<float> data, std::string unit)
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