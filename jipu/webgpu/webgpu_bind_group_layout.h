
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/bind_group_layout.h"
#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUBindGroupLayout : public RefCounted
{

public:
    static WebGPUBindGroupLayout* create(WebGPUDevice* wgpuDevice, WGPUBindGroupLayoutDescriptor const* descriptor);

public:
    WebGPUBindGroupLayout() = delete;
    explicit WebGPUBindGroupLayout(WebGPUDevice* device, std::unique_ptr<BindGroupLayout> layout, WGPUBindGroupLayoutDescriptor const* descriptor);

public:
    virtual ~WebGPUBindGroupLayout() = default;

    WebGPUBindGroupLayout(const WebGPUBindGroupLayout&) = delete;
    WebGPUBindGroupLayout& operator=(const WebGPUBindGroupLayout&) = delete;

public: // WebGPU API
public:
    BindGroupLayout* getBindGroupLayout() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUBindGroupLayoutDescriptor m_descriptor{};

private:
    std::unique_ptr<BindGroupLayout> m_layout = nullptr;
};

// Conert from JIPU to WebGPU
WGPUShaderStage ToWGPUShaderStage(BindingStageFlags stages);
WGPUBufferBindingType ToWGPUBufferBindingType(BufferBindingType type);
WGPUStorageTextureAccess ToWGPUStorageTextureAccess(StorageTextureAccess access);

// Convert from WebGPU to JIPU
BindingStageFlags WGPUToBindingStageFlags(WGPUShaderStage stages);
BufferBindingType WGPUToBufferBindingType(WGPUBufferBindingType type);
StorageTextureAccess WGPUToStorageTextureAccess(WGPUStorageTextureAccess access);

} // namespace jipu