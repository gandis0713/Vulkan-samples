#include "request_adapter_event.h"

#include "jipu/webgpu/webgpu_adapter.h"

namespace jipu
{

std::unique_ptr<RequestAdapterEvent> RequestAdapterEvent::create(WebGPUAdapter* adapter, WGPURequestAdapterCallbackInfo2 callbackInfo)
{
    return std::unique_ptr<RequestAdapterEvent>(new RequestAdapterEvent(adapter, callbackInfo));
}

RequestAdapterEvent::RequestAdapterEvent(WebGPUAdapter* adapter, WGPURequestAdapterCallbackInfo2 callbackInfo)
    : Event(callbackInfo.mode)
    , m_adapter(adapter)
    , m_callbackInfo(callbackInfo)
{
}

void RequestAdapterEvent::complete()
{
    if (m_adapter == nullptr)
    {
        std::string message{ "Failed to request adapter." };
        m_callbackInfo.callback(WGPURequestAdapterStatus_Error, nullptr, WGPUStringView{ .data = nullptr, .length = 0 }, m_callbackInfo.userdata1, m_callbackInfo.userdata2);
        return;
    }

    m_callbackInfo.callback(WGPURequestAdapterStatus_Success, reinterpret_cast<WGPUAdapter>(m_adapter), WGPUStringView{ .data = nullptr, .length = 0 }, m_callbackInfo.userdata1, m_callbackInfo.userdata2);
}

} // namespace jipu