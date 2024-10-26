#pragma once

#include "export.h"

#include "texture.h"

namespace jipu
{

enum class PresentMode
{
    kUndefined = 0x00000000,
    kImmediate,
    kFifo,
    kFifoRelaxed,
    kMailbox,
};

enum class ColorSpace
{
    kUndefined = 0x00000000,
    kSRGBNonLinear,
    kSRGBLinear,
};

enum class CompositeAlphaFlag
{
    kUndefined = 0x00000000,
    kOpaque,
    kPreMultiplied,
    kPostMultiplied,
    kInherit,
};

struct SurfaceCapabilities
{
    std::vector<TextureFormat> formats{};
    std::vector<PresentMode> presentModes{};
    std::vector<CompositeAlphaFlag> compositeAlphaFlags{};
};

struct SurfaceDescriptor
{
    void* windowHandle;
};

class JIPU_EXPORT Surface
{
public:
    virtual ~Surface() = default;

    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

public:
    virtual void present() = 0;

protected:
    Surface() = default;
};

}; // namespace jipu
