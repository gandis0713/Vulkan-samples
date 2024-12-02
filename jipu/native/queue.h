#pragma once

#include "export.h"

#include "command_buffer.h"
#include "swapchain.h"

#include <functional>
#include <stdint.h>
#include <vector>

namespace jipu
{

struct QueueDescriptor
{
};

class JIPU_EXPORT Queue
{
public:
    virtual ~Queue() = default;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

public:
    virtual void submit(std::vector<CommandBuffer*> commandBuffers) = 0;

protected:
    Queue() = default;
};

} // namespace jipu