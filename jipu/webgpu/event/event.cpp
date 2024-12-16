#include "event.h"

namespace jipu
{

Event::Event(WGPUCallbackMode mode)
    : m_mode(mode)
{
}

WGPUCallbackMode Event::getMode() const
{
    return m_mode;
}

bool Event::isCompleted() const
{
    return m_isCompleted.load();
}

} // namespace jipu