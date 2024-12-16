#include "webgpu_instance.h"

#include "event/request_adapter_event.h"
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
    , m_eventManager(std::make_unique<EventManager>())
{
}

WGPUWaitStatus WebGPUInstance::waitAny(const uint64_t waitCount, WGPUFutureWaitInfo* waitInfos, uint64_t timeoutNS)
{
    // TODO: timeout
    return m_eventManager->waitAny(waitCount, waitInfos);
}

WGPUFuture WebGPUInstance::requestAdapter(WGPURequestAdapterOptions const* options, WGPURequestAdapterCallbackInfo2 callbackInfo)
{
    auto adapter = WebGPUAdapter::create(this, options);
    auto event = RequestAdapterEvent::create(adapter, callbackInfo);

    return WGPUFuture{ .id = static_cast<uint64_t>(m_eventManager->addEvent(std::move(event))) };
}

WebGPUSurface* WebGPUInstance::createSurface(WGPUSurfaceDescriptor const* descriptor)
{
    return new WebGPUSurface(this, descriptor);
}

Instance* WebGPUInstance::getInstance() const
{
    return m_instance.get();
}

EventManager* WebGPUInstance::getEventManager() const
{
    return m_eventManager.get();
}

} // namespace jipu