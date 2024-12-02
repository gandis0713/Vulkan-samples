
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/bind_group.h"
#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUBindGroup : public RefCounted
{

public:
    static WebGPUBindGroup* create(WebGPUDevice* wgpuDevice, WGPUBindGroupDescriptor const* descriptor);

public:
    WebGPUBindGroup() = delete;
    explicit WebGPUBindGroup(WebGPUDevice* wgpuDevice, std::unique_ptr<BindGroup> layout, WGPUBindGroupDescriptor const* descriptor);

public:
    virtual ~WebGPUBindGroup() = default;

    WebGPUBindGroup(const WebGPUBindGroup&) = delete;
    WebGPUBindGroup& operator=(const WebGPUBindGroup&) = delete;

public: // WebGPU API
public:
    BindGroup* getBindGroup() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUBindGroupDescriptor m_descriptor{};

private:
    std::unique_ptr<BindGroup> m_bindGroup = nullptr;
};

} // namespace jipu