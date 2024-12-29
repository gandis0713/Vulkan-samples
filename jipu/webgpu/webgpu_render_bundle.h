#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/render_bundle.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPURenderBundleEncoder;
class WebGPURenderBundle : public RefCounted
{
public:
    static WebGPURenderBundle* create(WebGPURenderBundleEncoder* wgpuRenderBundleEncoder, WGPURenderBundleDescriptor const* descriptor);

public:
    WebGPURenderBundle() = delete;
    explicit WebGPURenderBundle(WebGPURenderBundleEncoder* wgpuRenderBundleEncoder, std::unique_ptr<RenderBundle> renderBundle, WGPURenderBundleDescriptor const* descriptor);

public:
    virtual ~WebGPURenderBundle() = default;

    WebGPURenderBundle(const WebGPURenderBundle&) = delete;
    WebGPURenderBundle& operator=(const WebGPURenderBundle&) = delete;

private:
    // WebGPURenderBundleEncoder* m_wgpuRenderBundleEncoder = nullptr;
    [[maybe_unused]] const WGPURenderBundleDescriptor m_descriptor{};

private:
    std::unique_ptr<RenderBundle> m_renderBundle = nullptr;
};

} // namespace jipu