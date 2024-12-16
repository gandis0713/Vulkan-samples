#pragma once

#include "export.h"
#include "render_bundle.h"

#include <memory>

namespace jipu
{

struct RenderBundleEncoderDescriptor
{
};

class JIPU_EXPORT RenderBundleEncoder
{
public:
    virtual ~RenderBundleEncoder() = default;

    RenderBundleEncoder(const RenderBundleEncoder&) = delete;
    RenderBundleEncoder& operator=(const RenderBundleEncoder&) = delete;

public:
    virtual std::unique_ptr<RenderBundle> finish(const RenderBundleDescriptor& descriptor) = 0;

protected:
    RenderBundleEncoder() = default;
};

} // namespace jipu