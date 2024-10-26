#pragma once

#include "jipu/common/ref_counted.h"
#include "webgpu_header.h"

namespace jipu
{

class WebGPUSurface;
class WebGPUInstance : public RefCounted
{
public:
    static WebGPUInstance* create(WGPUInstanceDescriptor const* wgpuDescriptor);

public:
    WebGPUInstance();
    explicit WebGPUInstance(WGPUInstanceDescriptor const* wgpuDescriptor);

public:
    virtual ~WebGPUInstance() = default;

    WebGPUInstance(const WebGPUInstance&) = delete;
    WebGPUInstance& operator=(const WebGPUInstance&) = delete;

public: // WebGPU API
    void requestAdapter(WGPURequestAdapterOptions const* options, WGPURequestAdapterCallback callback, void* userdata);
    WebGPUSurface* createSurface(WGPUSurfaceDescriptor const* descriptor);

public:
    [[maybe_unused]] const WGPUInstanceDescriptor m_wgpuDescriptor{};
};

} // namespace jipu