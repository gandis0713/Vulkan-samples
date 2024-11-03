#pragma once

#include "jipu/common/dylib.h"

#include "webgpu_api.h"
#include "window.h"

#include <filesystem>
#include <unordered_map>

namespace jipu
{

struct WGPUSampleDescriptor
{
    WindowDescriptor windowDescriptor;
    std::filesystem::path path;
};

class WGPUSample : public Window
{
public:
    WGPUSample() = delete;
    WGPUSample(const WGPUSampleDescriptor& descriptor);
    virtual ~WGPUSample();

public:
    void init() override;
    void update() override;

protected:
    enum class LibType
    {
        kJipu = 0,
        kDawn,
        kCount
    };

    void setLibType(LibType type);
    WGPUSample::LibType getLibType();

    WebGPUAPI& wgpu();
    WebGPUAPI& wgpu(LibType type);

protected:
    std::filesystem::path m_appPath;
    std::filesystem::path m_appDir;

    LibType m_libType{ LibType::kJipu };

    std::unordered_map<LibType, DyLib> m_wgpuLibs{};
    std::unordered_map<LibType, WebGPUAPI> m_wgpuAPIs{};
};

} // namespace jipu
