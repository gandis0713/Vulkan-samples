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

void WGPUSample::update()
{
}

void WGPUSample::changeAPI(WGPUSample::APIType type)
{
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
        .backendType = getAPIType() == APIType::kDawn ? WGPUBackendType_Metal : WGPUBackendType_Vulkan,
#elif defined(WIN32)
        .backendType = WGPUBackendType_D3D12,
#endif
        .forceFallbackAdapter = false,
    };

    wgpu.InstanceRequestAdapter(m_instance, &descriptor, cb, &m_adapter);

    assert(m_adapter);
}

void WGPUSample::createDevice()
{
    auto cb = [](WGPURequestDeviceStatus status, WGPUDevice device, struct WGPUStringView message, void* userdata) {
        if (status != WGPURequestDeviceStatus_Success)
        {
            throw std::runtime_error("Failed to request device.");
        }

        *static_cast<WGPUDevice*>(userdata) = device;
    };

    WGPUDeviceDescriptor deviceDescriptor{};

    wgpu.AdapterRequestDevice(m_adapter, &deviceDescriptor, cb, &m_device);

    assert(m_device);
}

void WGPUSample::createSurfaceConfigure()
{
    auto status = wgpu.SurfaceGetCapabilities(m_surface, m_adapter, &m_surfaceCapabilities);
    if (status != WGPUStatus_Success)
        throw std::runtime_error("Failed to get surface capabilities.");

    auto& surfaceCapabilities = m_surfaceCapabilities;

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

} // namespace jipu