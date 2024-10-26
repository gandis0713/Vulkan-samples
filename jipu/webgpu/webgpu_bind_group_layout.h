
#pragma once

#include "webgpu_header.h"

#include "jipu/common/ref_counted.h"

#include "jipu/native/binding_group_layout.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUBindGroupLayout : public RefCounted
{

public:
    static WebGPUBindGroupLayout* create(WebGPUDevice* wgpuDevice, WGPUBindGroupLayoutDescriptor const* descriptor);

public:
    WebGPUBindGroupLayout() = delete;
    explicit WebGPUBindGroupLayout(WebGPUDevice* device, std::unique_ptr<BindingGroupLayout> layout, WGPUBindGroupLayoutDescriptor const* descriptor);

public:
    virtual ~WebGPUBindGroupLayout() = default;

    WebGPUBindGroupLayout(const WebGPUBindGroupLayout&) = delete;
    WebGPUBindGroupLayout& operator=(const WebGPUBindGroupLayout&) = delete;

public: // WebGPU API
public:
    BindingGroupLayout* getBindingGroupLayout() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUBindGroupLayoutDescriptor m_descriptor{};

private:
    std::unique_ptr<BindingGroupLayout> m_layout = nullptr;
};

// Convert from WebGPU to JIPU
BindingStageFlags ToBindingStageFlags(WGPUShaderStage stages);
BufferBindingType ToBufferBindingType(WGPUBufferBindingType type);

// Conert from JIPU to WebGPU
WGPUShaderStage ToWGPUShaderStage(BindingStageFlags stages);
WGPUBufferBindingType ToWGPUBufferBindingType(BufferBindingType type);

} // namespace jipu