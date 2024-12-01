#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/webgpu/webgpu_header.h"

#include "jipu/native/instance.h"

namespace jipu
{

class WebGPUSurface;
class WebGPUInstance : public RefCounted
{
public:
    static WebGPUInstance* create(WGPUInstanceDescriptor const* wgpuDescriptor);

public:
    virtual ~WebGPUInstance() = default;

    WebGPUInstance(const WebGPUInstance&) = delete;
    WebGPUInstance& operator=(const WebGPUInstance&) = delete;

public: // WebGPU API
    void requestAdapter(WGPURequestAdapterOptions const* options, WGPURequestAdapterCallback callback, void* userdata);
    WebGPUSurface* createSurface(WGPUSurfaceDescriptor const* descriptor);

public:
    Instance* getInstance() const;

private:
    [[maybe_unused]] const WGPUInstanceDescriptor m_wgpuDescriptor{};

private:
    std::unique_ptr<Instance> m_instance = nullptr;

private:
    explicit WebGPUInstance(std::unique_ptr<Instance> instance, const WGPUInstanceDescriptor& wgpuDescriptor);
};

} // namespace jipu