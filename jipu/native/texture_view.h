#pragma once

#include "export.h"
#include <functional>
#include <stdint.h>

namespace jipu
{

enum class TextureViewDimension
{
    kUndefined = 0,
    k1D,
    k2D,
    k2DArray,
    k3D,
    kCube,
    kCubeArray,
};

struct TextureAspectFlagBits
{
    static constexpr uint32_t kUndefined = 0x00000000;
    static constexpr uint32_t kColor = 0x00000001;
    static constexpr uint32_t kDepth = 0x00000002;
    static constexpr uint32_t kStencil = 0x00000004;
};
using TextureAspectFlags = uint32_t;

struct TextureViewDescriptor
{
    TextureViewDimension dimension = TextureViewDimension::kUndefined;
    TextureAspectFlags aspect = TextureAspectFlagBits::kUndefined;
};

class Texture;
class JIPU_EXPORT TextureView
{
public:
    virtual ~TextureView() = default;

    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

public:
    virtual Texture* getTexture() const = 0;
    virtual TextureViewDimension getDimension() const = 0;
    virtual TextureAspectFlags getAspect() const = 0;
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getDepth() const = 0;

protected:
    TextureView() = default;

public:
    using Ref = std::reference_wrapper<TextureView>;
};

} // namespace jipu
