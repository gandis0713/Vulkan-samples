#include "webgpu_render_bundle.h"

namespace jipu
{

WebGPURenderBundle* WebGPURenderBundle::create(WebGPURenderBundleEncoder* wgpuRenderBundleEncoder, WGPURenderBundleDescriptor const* descriptor)
{
    return nullptr;
}

WebGPURenderBundle::WebGPURenderBundle(WebGPURenderBundleEncoder* wgpuRenderBundleEncoder, std::unique_ptr<RenderBundle> renderBundle, WGPURenderBundleDescriptor const* descriptor)
    // : m_wgpuRenderBundleEncoder(wgpuRenderBundleEncoder)
    : m_descriptor(*descriptor)
    , m_renderBundle(std::move(renderBundle))
{
}

} // namespace jipu