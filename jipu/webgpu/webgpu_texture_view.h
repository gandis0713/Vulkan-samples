
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/texture_view.h"
#include "webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUTexture;
class WebGPUTextureView : public RefCounted
{

public:
    static WebGPUTextureView* create(WebGPUTexture* texture, WGPUTextureViewDescriptor const* descriptor);

public:
    WebGPUTextureView() = delete;
    explicit WebGPUTextureView(WebGPUTexture* texture, std::unique_ptr<TextureView> textureView, WGPUTextureViewDescriptor const* descriptor);

public:
    virtual ~WebGPUTextureView() override = default;

    WebGPUTextureView(const WebGPUTextureView&) = delete;
    WebGPUTextureView& operator=(const WebGPUTextureView&) = delete;

public: // WebGPU API
public:
    TextureView* getTextureView() const;

private:
    [[maybe_unused]] WebGPUTexture* m_wgpuTexture = nullptr;
    [[maybe_unused]] const WGPUTextureViewDescriptor m_descriptor{};

private:
    std::unique_ptr<TextureView> m_textureView = nullptr;
};

// Generators
WGPUTextureViewDescriptor GenerateWGPUTextureViewDescriptor(WebGPUTexture* wgpuTexture);

// Convert from WebGPU to JIPU
WGPUTextureViewDimension ToWGPUTextureViewDimension(TextureViewDimension dimension);
WGPUTextureAspect ToWGPUTextureAspect(TextureAspectFlags aspect);

// Convert from JIPU to WebGPU
TextureViewDimension ToTextureViewDimension(WGPUTextureViewDimension dimension);
TextureAspectFlags ToTextureAspectFlags(WebGPUTexture* wgpuTexture, WGPUTextureAspect aspect);

} // namespace jipu