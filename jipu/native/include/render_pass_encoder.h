#pragma once

#include "query_set.h"
#include "texture_view.h"

#include "export.h"
#include <optional>
#include <vector>

namespace jipu
{

class Pipeline;
class Buffer;
class CommandBuffer;
class BindingGroup;

enum class LoadOp : uint8_t
{
    kDontCare = 0,
    kLoad,
    kClear,
    kExpandResolveTexture,
};

enum class StoreOp : uint8_t
{
    kDontCare = 0,
    kStore,
};

struct Color
{
    double r;
    double g;
    double b;
    double a;
};

struct ColorAttachment
{
    TextureView* renderView = nullptr;
    TextureView* resolveView = nullptr;
    LoadOp loadOp = LoadOp::kDontCare;
    StoreOp storeOp = StoreOp::kDontCare;
    Color clearValue{};
};

struct DepthStencilClearValue
{
    float depth = 1.0f;
    uint32_t stencil = 0;
};

struct DepthStencilAttachment
{
    TextureView* textureView = nullptr;
    LoadOp depthLoadOp = LoadOp::kDontCare;
    StoreOp depthStoreOp = StoreOp::kDontCare;
    LoadOp stencilLoadOp = LoadOp::kDontCare;
    StoreOp stencilStoreOp = StoreOp::kDontCare;
    DepthStencilClearValue clearValue{};
};

struct RenderPassTimestampWrites
{
    QuerySet* querySet = nullptr;
    uint32_t beginQueryIndex = 0;
    uint32_t endQueryIndex = 0;
};

struct RenderPassEncoderDescriptor
{
    std::vector<ColorAttachment> colorAttachments{};
    std::optional<DepthStencilAttachment> depthStencilAttachment = std::nullopt;
    QuerySet* occlusionQuerySet = nullptr;
    RenderPassTimestampWrites timestampWrites{};
};

enum IndexFormat
{
    kUndefined = 0,
    kUint16,
    kUint32,
};

class RenderPipeline;
class JIPU_EXPORT RenderPassEncoder
{
public:
    virtual ~RenderPassEncoder() = default;

protected:
    RenderPassEncoder() = default;

public:
    virtual void setPipeline(RenderPipeline* pipeline) = 0;
    virtual void setBindingGroup(uint32_t index, BindingGroup& bindingGroup, std::vector<uint32_t> dynamicOffset = {}) = 0;

    virtual void setVertexBuffer(uint32_t slot, Buffer& buffer) = 0;
    virtual void setIndexBuffer(Buffer& buffer, IndexFormat format) = 0;

    virtual void setViewport(float x,
                             float y,
                             float width,
                             float height,
                             float minDepth,
                             float maxDepth) = 0;

    virtual void setScissor(float x,
                            float y,
                            float width,
                            float height) = 0;
    virtual void setBlendConstant(const Color& color) = 0;

    virtual void draw(uint32_t vertexCount,
                      uint32_t instanceCount,
                      uint32_t firstVertex,
                      uint32_t firstInstance) = 0;
    virtual void drawIndexed(uint32_t indexCount,
                             uint32_t instanceCount,
                             uint32_t indexOffset,
                             uint32_t vertexOffset,
                             uint32_t firstInstance) = 0;

    virtual void beginOcclusionQuery(uint32_t queryIndex) = 0;
    virtual void endOcclusionQuery() = 0;

    virtual void end() = 0;
};

} // namespace jipu