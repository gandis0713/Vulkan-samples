
#pragma once

#include <memory>

#include "jipu/common/ref_counted.h"
#include "jipu/native/shader_module.h"
#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUShaderModule : public RefCounted
{

public:
    static WebGPUShaderModule* create(WebGPUDevice* wgpuDevice, WGPUShaderModuleDescriptor const* descriptor);

public:
    WebGPUShaderModule() = delete;
    explicit WebGPUShaderModule(WebGPUDevice* wgpuDevice, std::unique_ptr<ShaderModule> shaderModule, WGPUShaderModuleDescriptor const* descriptor);

public:
    virtual ~WebGPUShaderModule() = default;

    WebGPUShaderModule(const WebGPUShaderModule&) = delete;
    WebGPUShaderModule& operator=(const WebGPUShaderModule&) = delete;

public: // WebGPU API
public:
    ShaderModule* getShaderModule() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUShaderModuleDescriptor m_descriptor{};

private:
    std::unique_ptr<ShaderModule> m_shaderModule = nullptr;
};

} // namespace jipu