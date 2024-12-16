#pragma once

#include "event.h"

#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUDevice;
class RequestDeviceEvent : public Event
{
public:
    static std::unique_ptr<RequestDeviceEvent> create(WebGPUDevice* device, WGPURequestDeviceCallbackInfo2 callbackInfo);

public:
    RequestDeviceEvent() = delete;
    virtual ~RequestDeviceEvent() = default;

public:
    void complete() override;

private:
    RequestDeviceEvent(WebGPUDevice* device, WGPURequestDeviceCallbackInfo2 callbackInfo);

private:
    WebGPUDevice* m_device{ nullptr };
    WGPURequestDeviceCallbackInfo2 m_callbackInfo{ WGPU_REQUEST_DEVICE_CALLBACK_INFO_2_INIT };
};

} // namespace jipu