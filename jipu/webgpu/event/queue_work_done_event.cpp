#include "queue_work_done_event.h"

#include "jipu/webgpu/webgpu_device.h"

namespace jipu
{

std::unique_ptr<QueueWorkDoneEvent> QueueWorkDoneEvent::create(WGPUQueueWorkDoneCallbackInfo2 callbackInfo)
{
    return std::unique_ptr<QueueWorkDoneEvent>(new QueueWorkDoneEvent(callbackInfo));
}

QueueWorkDoneEvent::QueueWorkDoneEvent(WGPUQueueWorkDoneCallbackInfo2 callbackInfo)
    : Event(callbackInfo.mode)
    , m_callbackInfo(callbackInfo)
{
}

void QueueWorkDoneEvent::complete()
{
    if (m_isCompleted.load())
    {
        return;
    }

    m_callbackInfo.callback(WGPUQueueWorkDoneStatus_Success, m_callbackInfo.userdata1, m_callbackInfo.userdata2);
    m_isCompleted.store(true);
}

} // namespace jipu