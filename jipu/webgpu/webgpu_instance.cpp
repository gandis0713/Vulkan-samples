#include "webgpu_instance.h"

#include "webgpu_adapter.h"
#include "webgpu_surface.h"

namespace jipu
{

WebGPUInstance* WebGPUInstance::create(WGPUInstanceDescriptor const* wgpuDescriptor)
{
    WGPUInstanceDescriptor descriptor = wgpuDescriptor ? *wgpuDescriptor : WGPUInstanceDescriptor{};

    auto instance = Instance::create({});
    if (!instance)
    {
        // TODO: log or throw error
        return nullptr;
    }

    return new WebGPUInstance(std::move(instance), descriptor);
}

WebGPUInstance::WebGPUInstance(std::unique_ptr<Instance> instance, const WGPUInstanceDescriptor& wgpuDescriptor)
    : m_wgpuDescriptor(wgpuDescriptor)
    , m_instance(std::move(instance))
{
}

void WebGPUInstance::requestAdapter(WGPURequestAdapterOptions const* options, WGPURequestAdapterCallback callback, void* userdata)
{
    auto adapter = WebGPUAdapter::create(this, options);
    if (adapter)
    {
        std::string message = "Succeed to create adapter";
        callback(WGPURequestAdapterStatus::WGPURequestAdapterStatus_Success, reinterpret_cast<WGPUAdapter>(adapter), WGPUStringView{ .data = message.data(), .length = message.size() }, userdata);
    }
    else
    {
        std::string message = "Failed to create adapter";
        callback(WGPURequestAdapterStatus::WGPURequestAdapterStatus_Error, nullptr, WGPUStringView{ .data = message.data(), .length = message.size() }, userdata);
    }
}

WebGPUSurface* WebGPUInstance::createSurface(WGPUSurfaceDescriptor const* descriptor)
{
    return new WebGPUSurface(this, descriptor);
}

Instance* WebGPUInstance::getInstance() const
{
    return m_instance.get();
}

} // namespace jipu