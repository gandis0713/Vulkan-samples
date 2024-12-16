#pragma once

#include <unordered_map>

#include "event.h"

namespace jipu
{

using FutureID = uint64_t;
class EventManager
{
public:
    EventManager() = default;
    virtual ~EventManager() = default;

public:
    WGPUWaitStatus waitAny(const uint64_t waitCount, WGPUFutureWaitInfo* waitInfos);
    void processEvents();

public:
    FutureID addEvent(std::unique_ptr<Event> event);

private:
    FutureID generateId();

private:
    std::unordered_map<FutureID, std::unique_ptr<Event>> m_events{};
    FutureID m_currentId{ 0 };
};

} // namespace jipu