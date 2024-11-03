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

    initializeContext();
}

WGPUSample::APIType WGPUSample::getAPIType()
{
    return m_apiType;
}

WebGPUAPI& WGPUSample::wgpu()
{
    return m_wgpuAPIs[m_apiType];
}

WebGPUAPI& WGPUSample::wgpu(APIType type)
{
    return m_wgpuAPIs[type];
}

} // namespace jipu