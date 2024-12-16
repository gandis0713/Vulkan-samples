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

            m_events.erase(waitInfo.future.id);
        }
    }

    // TODO: return if first event is completed
    return WGPUWaitStatus_Success;
}

void EventManager::processEvents()
{
    for (auto it = m_events.begin(); it != m_events.end();)
    {
        if (it->second->getMode() == WGPUCallbackMode_AllowProcessEvents)
        {
            it->second->complete();
            it = m_events.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

FutureID EventManager::addEvent(std::unique_ptr<Event> event)
{
    FutureID id = generateId();

    if (event->getMode() == WGPUCallbackMode_AllowSpontaneous)
    {
        event->complete();
        return id;
    }

    m_events[id] = std::move(event);
    return id;
}

FutureID EventManager::generateId()
{
    return m_currentId++;
}

} // namespace jipu