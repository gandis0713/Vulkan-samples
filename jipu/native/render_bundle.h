#pragma once

#include "export.h"

namespace jipu
{

struct RenderBundleDescriptor
{
};

class JIPU_EXPORT RenderBundle
{
public:
    virtual ~RenderBundle() = default;

    RenderBundle(const RenderBundle&) = delete;
    RenderBundle& operator=(const RenderBundle&) = delete;

protected:
    RenderBundle() = default;
};

} // namespace jipu