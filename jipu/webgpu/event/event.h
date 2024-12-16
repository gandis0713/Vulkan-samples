#pragma once

#include "jipu/webgpu/webgpu_header.h"

#include <atomic>

namespace jipu
{

class Event
{
public:
    // Event() = delete;
    virtual ~Event() = default;

protected:
    Event(WGPUCallbackMode mode);

public:
    virtual void complete() = 0;
    bool isCompleted() const;

public:
    WGPUCallbackMode getMode() const;

protected:
    std::atomic<bool> m_isCompleted{ false };

private:
    WGPUCallbackMode m_mode{ WGPUCallbackMode_WaitAnyOnly };
};

} // namespace jipu