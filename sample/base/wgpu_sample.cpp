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
    std::unordered_map<LibType, std::string> wgpuLibNames{
        { LibType::kJipu, "libjipu.so" },
        { LibType::kDawn, "libwebgpu_dawn.so" },
    };
#elif defined(__linux__)
    std::unordered_map<LibType, std::string> wgpuLibNames{
        { LibType::kJipu, "libjipu.so" },
        { LibType::kDawn, "libwebgpu_dawn.so" },
    };
#elif defined(__APPLE__)
    std::unordered_map<LibType, std::string> wgpuLibNames{
        { LibType::kJipu, "libjipu.dylib" },
        { LibType::kDawn, "libwebgpu_dawn.dylib" },
    };
#elif defined(WIN32)
    std::unordered_map<LibType, std::string> wgpuLibNames{
        { LibType::kJipu, "jipu.dll" },
        { LibType::kDawn, "webgpu_dawn.dll" },
    };
#endif

    for (auto i = 0; i < static_cast<uint16_t>(LibType::kCount); ++i)
    {
        auto type = static_cast<LibType>(i);

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

void WGPUSample::setLibType(WGPUSample::LibType type)
{
    m_libType = type;
}

WGPUSample::LibType WGPUSample::getLibType()
{
    return m_libType;
}

WebGPUAPI& WGPUSample::wgpu()
{
    return m_wgpuAPIs[m_libType];
}

WebGPUAPI& WGPUSample::wgpu(LibType type)
{
    return m_wgpuAPIs[type];
}

} // namespace jipu