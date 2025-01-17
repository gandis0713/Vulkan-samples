#pragma once

#include "export.h"
#include <optional>
#include <vector>

namespace jipu
{

enum class BufferBindingType
{
    kUndefined = 0,
    kUniform,
    kStorage,
    kReadOnlyStorage,
};

enum class StorageTextureBindingType
{
    kUndefined = 0,
    kWriteOnly,
    kReadOnly,
    kReadWrite,
};

struct BindingStageFlagBits
{
    static constexpr uint32_t kUndefined = 1 << 0;     // 0x00000000
    static constexpr uint32_t kVertexStage = 1 << 1;   // 0x00000001
    static constexpr uint32_t kFragmentStage = 1 << 2; // 0x00000002
    static constexpr uint32_t kComputeStage = 1 << 3;  // 0x00000004
};
using BindingStageFlags = uint32_t;

struct BufferBindingLayout
{
    /// @brief The index of binding.
    uint32_t index = 0;
    BindingStageFlags stages = 0u;
    BufferBindingType type = BufferBindingType::kUndefined;
    bool dynamicOffset = false;
};

struct SamplerBindingLayout
{
    /// @brief The index of binding.
    uint32_t index = 0;
    BindingStageFlags stages = 0u;
};

struct TextureBindingLayout
{
    /// @brief The index of binding.
    uint32_t index = 0;
    BindingStageFlags stages = 0u;
};

struct StorageTextureBindingLayout
{
    /// @brief The index of binding.
    uint32_t index = 0;
    BindingStageFlags stages = 0u;
    StorageTextureBindingType type = StorageTextureBindingType::kUndefined;
};

struct BindGroupLayoutDescriptor
{
    std::vector<BufferBindingLayout> buffers = {};
    std::vector<SamplerBindingLayout> samplers = {};
    std::vector<TextureBindingLayout> textures = {};
    std::vector<StorageTextureBindingLayout> storageTextures = {};
};

class Device;
class JIPU_EXPORT BindGroupLayout
{
public:
    virtual ~BindGroupLayout() = default;

    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

public:
    virtual std::vector<BufferBindingLayout> getBufferBindingLayouts() const = 0;
    virtual std::vector<SamplerBindingLayout> getSamplerBindingLayouts() const = 0;
    virtual std::vector<TextureBindingLayout> getTextureBindingLayouts() const = 0;
    virtual std::vector<StorageTextureBindingLayout> getStorageTextureBindingLayouts() const = 0;

protected:
    BindGroupLayout() = default;
};

} // namespace jipu