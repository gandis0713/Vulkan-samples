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
};
} // namespace jipu