#include "request_device_event.h"

#include "jipu/webgpu/webgpu_device.h"

namespace jipu
{

std::unique_ptr<RequestDeviceEvent> RequestDeviceEvent::create(WebGPUDevice* device, WGPURequestDeviceCallbackInfo2 callbackInfo)
{
    return std::unique_ptr<RequestDeviceEvent>(new RequestDeviceEvent(device, callbackInfo));
}

RequestDeviceEvent::RequestDeviceEvent(WebGPUDevice* device, WGPURequestDeviceCallbackInfo2 callbackInfo)
    : Event(callbackInfo.mode)
    , m_device(device)
    , m_callbackInfo(callbackInfo)
{
}

void RequestDeviceEvent::complete()
{
    if (m_device == nullptr)
    {
        std::string message{ "Failed to request device." };
        m_callbackInfo.callback(WGPURequestDeviceStatus_Error, nullptr, WGPUStringView{ .data = nullptr, .length = 0 }, m_callbackInfo.userdata1, m_callbackInfo.userdata2);
        return;
    }

    m_callbackInfo.callback(WGPURequestDeviceStatus_Success, reinterpret_cast<WGPUDevice>(m_device), WGPUStringView{ .data = nullptr, .length = 0 }, m_callbackInfo.userdata1, m_callbackInfo.userdata2);
}

} // namespace jipu