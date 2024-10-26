
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/texture.h"
#include "webgpu_header.h"

namespace jipu
{

class WebGPUDevice;
class WebGPUTextureView;
class WebGPUTexture : public RefCounted
{

public:
    static WebGPUTexture* create(WebGPUDevice* wgpuDevice, Texture* texture);
    static WebGPUTexture* create(WebGPUDevice* wgpuDevice, WGPUTextureDescriptor const* descriptor);

public:
    WebGPUTexture() = delete;
    explicit WebGPUTexture(WebGPUDevice* wgpuDevice, Texture* texture);
    explicit WebGPUTexture(WebGPUDevice* wgpuDevice, std::unique_ptr<Texture> texture, WGPUTextureDescriptor const* descriptor);

public:
    virtual ~WebGPUTexture() override = default;

    WebGPUTexture(const WebGPUTexture&) = delete;
    WebGPUTexture& operator=(const WebGPUTexture&) = delete;

public: // WebGPU API
    WebGPUTextureView* createView(WGPUTextureViewDescriptor const* descriptor);

public:
    Texture* getTexture() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUTextureDescriptor m_descriptor{};

private:
    [[maybe_unused]] Texture* m_externalTexture = nullptr;            // For swapchain texture
    [[maybe_unused]] std::unique_ptr<Texture> m_ownTexture = nullptr; // For own texture
};

// Convert from WebGPU to JIPU
WGPUTextureFormat ToWGPUTextureFormat(TextureFormat format);

// Convert from JIPU to WebGPU
TextureFormat ToTextureFormat(WGPUTextureFormat format);

} // namespace jipu