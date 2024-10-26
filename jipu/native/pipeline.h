#pragma once

#include "export.h"
#include "pipeline_layout.h"
#include "shader_module.h"
#include "texture.h"

#include <optional>
#include <string>
#include <vector>

namespace jipu
{

class PipelineLayout;
class JIPU_EXPORT Pipeline
{
public:
    virtual ~Pipeline() = default;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

protected:
    Pipeline() = default;

public:
    virtual PipelineLayout* getPipelineLayout() const = 0;
};

struct ProgrammableStage
{
    ShaderModule* shaderModule = nullptr;
    std::string entryPoint = "main";
};

/**
 * @brief Render Pipeline
 */

// Input Assembly
enum class PrimitiveTopology
{
    kUndefined = 0,
    kPointList,
    kLineStrip,
    kLineList,
    kTriangleStrip,
    kTriangleList
};

struct InputAssemblyStage
{
    PrimitiveTopology topology = PrimitiveTopology::kUndefined;
};

// Vertex Stage
enum class VertexFormat
{
    kUndefined = 0,
    kUint8x2,
    kUint8x4,
    kSint8x2,
    kSint8x4,
    kUnorm8x2,
    kUnorm8x4,
    kSnorm8x2,
    kSnorm8x4,
    kUint16x2,
    kUint16x4,
    kSint16x2,
    kSint16x4,
    kUnorm16x2,
    kUnorm16x4,
    kSnorm16x2,
    kSnorm16x4,
    kFloat16x2,
    kFloat16x4,
    kFloat32,
    kFloat32x2,
    kFloat32x3,
    kFloat32x4,
    kUint32,
    kUint32x2,
    kUint32x3,
    kUint32x4,
    kSint32,
    kSint32x2,
    kSint32x3,
    kSint32x4,
    kUnorm10_10_10_2,
};

enum class VertexMode
{
    kVertex = 0,
    kVertexBufferNotUsed,
    kInstance,
};

struct VertexAttribute
{
    VertexFormat format = VertexFormat::kUndefined;
    uint64_t offset = 0u;
    uint32_t location = 0u;
    uint32_t slot = 0u;
};

struct VertexInputLayout
{
    VertexMode mode = VertexMode::kVertex;
    uint64_t stride = 0u;
    std::vector<VertexAttribute> attributes{};
};

struct VertexStage : ProgrammableStage
{
    std::vector<VertexInputLayout> layouts{};
};

// Rasterization Stage
enum class CullMode
{
    kNone,
    kFront,
    kBack
};

enum class FrontFace
{
    kCounterClockwise,
    kClockwise
};

struct RasterizationStage
{
    uint32_t sampleCount = 0;
    CullMode cullMode = CullMode::kNone;
    FrontFace frontFace = FrontFace::kCounterClockwise;
};

enum class BlendOperation
{
    kAdd = 0,
    kSubtract,
    kReversSubtract,
    kMin,
    kMax,
};

enum class BlendFactor
{
    kUndefined = 0,
    kZero,
    kOne,
    kSrcColor,
    kSrcAlpha,
    kSrcAlphaSaturated,
    kOneMinusSrcColor,
    kOneMinusSrcAlpha,
    kDstColor,
    kDstAlpha,
    kOneMinusDstColor,
    kOneMinusDstAlpha,
    kConstantColor,
    kOneMinusConstantColor,

    // dual source blending
    kSrc1Color,
    kOneMinusSrc1Color,
    kSrc1Alpha,
    kOneMinusSrc1Alpha,
};

struct BlendComponent
{
    BlendFactor srcFactor = BlendFactor::kOne;
    BlendFactor dstFactor = BlendFactor::kZero;
    BlendOperation operation = BlendOperation::kAdd;
};

struct BlendState
{
    BlendComponent color;
    BlendComponent alpha;
};

// Fragment Shader Stage
struct FragmentStage : ProgrammableStage
{
    struct Target
    {
        TextureFormat format = TextureFormat::kUndefined;
        // TextureUsageFlags usage = TextureUsageFlagBits::kUndefined;
        std::optional<BlendState> blend = std::nullopt;
    };

    std::vector<FragmentStage::Target> targets{};
};

// Depth/Stencil Stage
struct DepthStencilStage
{
    /// @brief depth/stencil attachment format.
    TextureFormat format;
};

struct RenderPipelineDescriptor
{
    /// @brief pipeline layout
    PipelineLayout* layout = nullptr;
    InputAssemblyStage inputAssembly{};
    VertexStage vertex;
    RasterizationStage rasterization{};
    FragmentStage fragment;
    std::optional<DepthStencilStage> depthStencil = std::nullopt;
};

class Device;
class JIPU_EXPORT RenderPipeline : public Pipeline
{
public:
    virtual ~RenderPipeline() = default;

    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

protected:
    RenderPipeline() = default;
};

// Compute Shader Stage
struct ComputeStage : ProgrammableStage
{
};

struct ComputePipelineDescriptor
{
    /// @brief pipeline layout
    PipelineLayout* layout = nullptr;
    ComputeStage compute;
};

class JIPU_EXPORT ComputePipeline : public Pipeline
{
public:
    virtual ~ComputePipeline() = default;

    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

protected:
    ComputePipeline() = default;
};

} // namespace jipu
