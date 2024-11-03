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

    virtual void initializeContext() = 0;
    virtual void finalizeContext() = 0;

protected:
    enum class APIType
    {
        kJipu = 0,
        kDawn,
        kCount
    };

    void changeAPI(APIType type);
    WGPUSample::APIType getAPIType();

    WebGPUAPI& wgpu();
    WebGPUAPI& wgpu(APIType type);

protected:
    std::filesystem::path m_appPath;
    std::filesystem::path m_appDir;

    APIType m_apiType{ APIType::kJipu };

    std::unordered_map<APIType, DyLib> m_wgpuLibs{};
    std::unordered_map<APIType, WebGPUAPI> m_wgpuAPIs{};
};

} // namespace jipu
