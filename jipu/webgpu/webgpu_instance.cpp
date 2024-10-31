#include "webgpu_instance.h"

#include "webgpu_adapter.h"
#include "webgpu_surface.h"

namespace jipu
{

WebGPUInstance* WebGPUInstance::create(WGPUInstanceDescriptor const* wgpuDescriptor)
{
    if (wgpuDescriptor)
    {
        return new WebGPUInstance(wgpuDescriptor);
    }

    return new WebGPUInstance();
}

WebGPUInstance::WebGPUInstance()
    : m_wgpuDescriptor()
{
}

WebGPUInstance::WebGPUInstance(WGPUInstanceDescriptor const* wgpuDescriptor)
    : m_wgpuDescriptor(*wgpuDescriptor)
{
}

void WebGPUInstance::requestAdapter(WGPURequestAdapterOptions const* options, WGPURequestAdapterCallback callback, void* userdata)
{
    auto adapter = WebGPUAdapter::create(this, options);
    if (adapter)
    {
        std::string message = "Succeed to create adapter";
        callback(WGPURequestAdapterStatus::WGPURequestAdapterStatus_Success, reinterpret_cast<WGPUAdapter>(adapter), WGPUStringView{ .data = message.data(), .length = message.length() }, userdata);
    }
    else
    {
        std::string message = "Failed to create adapter";
        callback(WGPURequestAdapterStatus::WGPURequestAdapterStatus_Error, nullptr, WGPUStringView{ .data = message.data(), .length = message.length() }, userdata);
    }
}

WebGPUSurface* WebGPUInstance::createSurface(WGPUSurfaceDescriptor const* descriptor)
{
    return new WebGPUSurface(this, descriptor);
}

} // namespace jipu