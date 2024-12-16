#pragma once

#include "event.h"

#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class QueueWorkDoneEvent : public Event
{
public:
    static std::unique_ptr<QueueWorkDoneEvent> create(WGPUQueueWorkDoneCallbackInfo2 callbackInfo);

public:
    QueueWorkDoneEvent() = delete;
    virtual ~QueueWorkDoneEvent() = default;

public:
    void complete() override;

private:
    QueueWorkDoneEvent(WGPUQueueWorkDoneCallbackInfo2 callbackInfo);

private:
    WGPUQueueWorkDoneCallbackInfo2 m_callbackInfo{ WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_2_INIT };
};

} // namespace jipu