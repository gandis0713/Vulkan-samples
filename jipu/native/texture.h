#pragma once

#include "export.h"
#include "texture_view.h"
#include <memory>
#include <stdint.h>

namespace jipu
{

struct Extent2D
{
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Extent3D
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;
};

enum class TextureType
{
    kUndefined = 0,
    k1D,
    k2D,
    k3D,
};

enum class TextureFormat
{
    kUndefined = 0,
    kR8Unorm = 1,
    kR8Snorm = 2,
    kR8Uint = 3,
    kR8Sint = 4,
    kR16Uint = 5,
    kR16Sint = 6,
    kR16Float = 7,
    kRG8Unorm = 8,
    kRG8Snorm = 9,
    kRG8Uint = 10,
    kRG8Sint = 11,
    kR32Float = 12,
    kR32Uint = 13,
    kR32Sint = 14,
    kRG16Uint = 15,
    kRG16Sint = 16,
    kRG16Float = 17,
    kRGBA8Unorm = 18,
    kRGBA8UnormSrgb = 19,
    kRGBA8Snorm = 20,
    kRGBA8Uint = 21,
    kRGBA8Sint = 22,
    kBGRA8Unorm = 23,
    kBGRA8UnormSrgb = 24,
    kRGB10A2Uint = 25,
    kRGB10A2Unorm = 26,
    kRG11B10Ufloat = 27,
    kRGB9E5Ufloat = 28,
    kRG32Float = 29,
    kRG32Uint = 30,
    kRG32Sint = 31,
    kRGBA16Uint = 32,
    kRGBA16Sint = 33,
    kRGBA16Float = 34,
    kRGBA32Float = 35,
    kRGBA32Uint = 36,
    kRGBA32Sint = 37,
    kStencil8 = 38,
    kDepth16Unorm = 39,
    kDepth24Plus = 40,
    kDepth24PlusStencil8 = 41,
    kDepth32Float = 42,
    kBC1RGBAUnorm = 43,
    kBC1RGBAUnormSrgb = 44,
    kBC2RGBAUnorm = 45,
    kBC2RGBAUnormSrgb = 46,
    kBC3RGBAUnorm = 47,
    kBC3RGBAUnormSrgb = 48,
    kBC4RUnorm = 49,
    kBC4RSnorm = 50,
    kBC5RGUnorm = 51,
    kBC5RGSnorm = 52,
    kBC6HRGBUfloat = 53,
    kBC6HRGBFloat = 54,
    kBC7RGBAUnorm = 55,
    kBC7RGBAUnormSrgb = 56,
    kETC2RGB8Unorm = 57,
    kETC2RGB8UnormSrgb = 58,
    kETC2RGB8A1Unorm = 59,
    kETC2RGB8A1UnormSrgb = 60,
    kETC2RGBA8Unorm = 61,
    kETC2RGBA8UnormSrgb = 62,
    kEACR11Unorm = 63,
    kEACR11Snorm = 64,
    kEACRG11Unorm = 65,
    kEACRG11Snorm = 66,
    kASTC4x4Unorm = 67,
    kASTC4x4UnormSrgb = 68,
    kASTC5x4Unorm = 69,
    kASTC5x4UnormSrgb = 70,
    kASTC5x5Unorm = 71,
    kASTC5x5UnormSrgb = 72,
    kASTC6x5Unorm = 73,
    kASTC6x5UnormSrgb = 74,
    kASTC6x6Unorm = 75,
    kASTC6x6UnormSrgb = 76,
    kASTC8x5Unorm = 77,
    kASTC8x5UnormSrgb = 78,
    kASTC8x6Unorm = 79,
    kASTC8x6UnormSrgb = 80,
    kASTC8x8Unorm = 81,
    kASTC8x8UnormSrgb = 82,
    kASTC10x5Unorm = 83,
    kASTC10x5UnormSrgb = 84,
    kASTC10x6Unorm = 85,
    kASTC10x6UnormSrgb = 86,
    kASTC10x8Unorm = 87,
    kASTC10x8UnormSrgb = 88,
    kASTC10x10Unorm = 89,
    kASTC10x10UnormSrgb = 90,
    kASTC12x10Unorm = 91,
    kASTC12x10UnormSrgb = 92,
    kASTC12x12Unorm = 93,
    kASTC12x12UnormSrgb = 94,
    kR16Unorm = 32768,
    kRG16Unorm = 32769,
    kRGBA16Unorm = 32770,
    kR16Snorm = 32771,
    kRG16Snorm = 32772,
    kRGBA16Snorm = 32773,
    kR8BG8Biplanar420Unorm = 32774,
    kR10X6BG10X6Biplanar420Unorm = 32775,
    kR8BG8A8Triplanar420Unorm = 32776,
    kR8BG8Biplanar422Unorm = 32777,
    kR8BG8Biplanar444Unorm = 32778,
    kR10X6BG10X6Biplanar422Unorm = 32779,
    kR10X6BG10X6Biplanar444Unorm = 32780,
    kExternal = 32781,
    kForce32 = 2147483647
};

struct TextureUsageFlagBits
{
    static constexpr uint32_t kUndefined = 0x00000000;
    static constexpr uint32_t kCopySrc = 0x00000001;
    static constexpr uint32_t kCopyDst = 0x00000002;
    static constexpr uint32_t kTextureBinding = 0x00000004;
    static constexpr uint32_t kStorageBinding = 0x00000008;
    static constexpr uint32_t kRenderAttachment = 0x00000010;
};
using TextureUsageFlags = uint32_t;

struct TextureDescriptor
{
    TextureType type = TextureType::kUndefined;
    TextureFormat format = TextureFormat::kUndefined;
    TextureUsageFlags usage = TextureUsageFlagBits::kUndefined;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 0;
    uint32_t mipLevels = 0;
    uint32_t sampleCount = 0;
};

class Device;
class JIPU_EXPORT Texture
{
public:
    virtual ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

public:
    virtual std::unique_ptr<TextureView> createTextureView(const TextureViewDescriptor& descriptor) = 0;

public:
    virtual TextureType getType() const = 0;
    virtual TextureFormat getFormat() const = 0;
    virtual TextureUsageFlags getUsage() const = 0;
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getDepth() const = 0;
    virtual uint32_t getMipLevels() const = 0;
    virtual uint32_t getSampleCount() const = 0;

protected:
    Texture() = default;
};

} // namespace jipu
