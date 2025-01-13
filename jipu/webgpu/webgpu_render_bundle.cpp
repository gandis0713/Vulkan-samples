#include "webgpu_render_bundle.h"

#include "webgpu_render_bundle_encoder.h"

namespace jipu
{

WebGPURenderBundle* WebGPURenderBundle::create(WebGPURenderBundleEncoder* wgpuRenderBundleEncoder, WGPURenderBundleDescriptor const* descriptor)
{
    RenderBundleDescriptor renderBundleDescriptor{};

    auto renderBundle = wgpuRenderBundleEncoder->getRenderBundleEncoder()->finish(renderBundleDescriptor);
    return new WebGPURenderBundle(wgpuRenderBundleEncoder, std::move(renderBundle), descriptor);
}

WebGPURenderBundle::WebGPURenderBundle(WebGPURenderBundleEncoder* wgpuRenderBundleEncoder, std::unique_ptr<RenderBundle> renderBundle, WGPURenderBundleDescriptor const* descriptor)
    // : m_wgpuRenderBundleEncoder(wgpuRenderBundleEncoder)
    : m_descriptor(*descriptor)
    , m_renderBundle(std::move(renderBundle))
{
}

RenderBundle* WebGPURenderBundle::getRenderBundle() const
{
    return m_renderBundle.get();
}

} // namespace jipu