#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/include/device.h"
#include "jipu/native/include/swapchain.h"
#include "jipu/native/include/texture.h"
#include "webgpu_header.h"

namespace jipu
{

class WebGPUAdapter;
class WebGPUQueue;
class WebGPUBindGroup;
class WebGPUBindGroupLayout;
class WebGPUPipelineLayout;
class WebGPURenderPipeline;
class WebGPUShaderModule;
class WebGPUTexture;
class WebGPUBuffer;
class WebGPUCommandEncoder;
class WebGPUDevice : public RefCounted
{
public:
    static WebGPUDevice* create(WebGPUAdapter* wgpuAdapter, WGPUDeviceDescriptor const* wgpuDescriptor);

public:
    WebGPUDevice() = delete;
    explicit WebGPUDevice(WebGPUAdapter* wgpuAdapter, std::unique_ptr<Device> device, WGPUDeviceDescriptor const* wgpuDescriptor);

public:
    ~WebGPUDevice() override;

    WebGPUDevice(const WebGPUDevice&) = delete;
    WebGPUDevice& operator=(const WebGPUDevice&) = delete;

public: // WebGPU API
    WebGPUQueue* getQueue();
    WebGPUBindGroup* createBindGroup(WGPUBindGroupDescriptor const* descriptor);
    WebGPUBindGroupLayout* createBindGroupLayout(WGPUBindGroupLayoutDescriptor const* descriptor);
    WebGPUPipelineLayout* createPipelineLayout(WGPUPipelineLayoutDescriptor const* descriptor);
    WebGPURenderPipeline* createRenderPipeline(WGPURenderPipelineDescriptor const* descriptor);
    WebGPUShaderModule* createShaderModule(WGPUShaderModuleDescriptor const* descriptor);
    WebGPUTexture* createTexture(Texture* texture);
    WebGPUTexture* createTexture(WGPUTextureDescriptor const* descriptor);
    WebGPUBuffer* createBuffer(WGPUBufferDescriptor const* descriptor);
    WebGPUCommandEncoder* createCommandEncoder(WGPUCommandEncoderDescriptor const* descriptor);

public:
    Device* getDevice() const;
    Swapchain* getOrCreateSwapchain(const SwapchainDescriptor& descriptor);

private:
    [[maybe_unused]] WebGPUAdapter* m_wgpuAdapter = nullptr;
    WebGPUQueue* m_wgpuQueue = nullptr;
    const WGPUDeviceDescriptor m_descriptor{};

private:
    /* TODO: Currently, the wgpu device have a only one swapchain in wgpu device.
             Consider swapchain pool or other ways. */
    std::pair<std::unique_ptr<Swapchain>, SwapchainDescriptor> m_swapchain{};
    std::unique_ptr<Device> m_device = nullptr;
};

// Generators
WGPUDeviceDescriptor GenerateWGPUDeviceDescriptor(WebGPUAdapter* wgpuAdapter);

} // namespace jipu