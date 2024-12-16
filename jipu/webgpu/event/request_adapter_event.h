#pragma once

#include "event.h"

#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUAdapter;
class RequestAdapterEvent : public Event
{
public:
    static std::unique_ptr<RequestAdapterEvent> create(WebGPUAdapter* adapter, WGPURequestAdapterCallbackInfo2 callbackInfo);

public:
    RequestAdapterEvent() = delete;
    virtual ~RequestAdapterEvent() = default;

public:
    void complete() override;

private:
    RequestAdapterEvent(WebGPUAdapter* adapter, WGPURequestAdapterCallbackInfo2 callbackInfo);

private:
    WebGPUAdapter* m_adapter{ nullptr };
    WGPURequestAdapterCallbackInfo2 m_callbackInfo{ WGPU_REQUEST_ADAPTER_CALLBACK_INFO_2_INIT };
};

} // namespace jipu