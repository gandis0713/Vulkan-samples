#pragma once

#include "export.h"

namespace jipu
{

struct CommandBufferDescriptor
{
};

class JIPU_EXPORT CommandBuffer
{
public:
    virtual ~CommandBuffer() = default;

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

protected:
    CommandBuffer() = default;
};
} // namespace jipu