#include "event_manager.h"

#include "jipu/webgpu/webgpu_header.h"

namespace jipu
{

WGPUWaitStatus EventManager::waitAny(const uint64_t waitCount, WGPUFutureWaitInfo* waitInfos)
{
    for (uint64_t i = 0; i < waitCount; ++i)
    {
        auto& waitInfo = waitInfos[i];
        auto event = m_events.find(waitInfo.future.id);
        if (event != m_events.end())
        {
            event->second->complete();
            waitInfo.completed = true;
        }
    }

    // TODO: return if first event is completed
    return WGPUWaitStatus_Success;
}

void EventManager::processEvents()
{
    for (auto& [_, event] : m_events)
    {
        if (event->getMode() == WGPUCallbackMode_AllowProcessEvents)
        {
            event->complete();
        }
    }
}

FutureID EventManager::addEvent(std::unique_ptr<Event> event)
{
    FutureID id = generateId();
    m_events[id] = std::move(event);
    return id;
}

FutureID EventManager::generateId()
{
    return m_currentId++;
}

} // namespace jipu