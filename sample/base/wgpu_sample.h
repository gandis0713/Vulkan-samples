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

    virtual void initializeContext();
    virtual void finalizeContext();

    virtual void createInstance();
    virtual void createSurface();
    virtual void createAdapter();
    virtual void createDevice();
    virtual void createSurfaceConfigure();
    virtual void createQueue();

protected:
    enum class APIType
    {
        kJipu = 0,
        kDawn,
        kCount
    };

    void changeAPI(APIType type);
    WGPUSample::APIType getAPIType();

protected:
    std::filesystem::path m_appPath;
    std::filesystem::path m_appDir;

    APIType m_apiType{ APIType::kJipu };

    std::unordered_map<APIType, DyLib> m_wgpuLibs{};
    std::unordered_map<APIType, WebGPUAPI> m_wgpuAPIs{};

protected:
    WGPUInstance m_instance = nullptr;
    WGPUSurface m_surface = nullptr;
    WGPUSurfaceCapabilities m_surfaceCapabilities{};
    WGPUSurfaceConfiguration m_surfaceConfigure{};
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUQueue m_queue = nullptr;

    WebGPUAPI wgpu{};
};

} // namespace jipu
