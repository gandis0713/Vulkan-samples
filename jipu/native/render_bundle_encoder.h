#pragma once

#include "buffer.h"
#include "export.h"
#include "render_bundle.h"

#include <memory>
#include <vector>

namespace jipu
{

struct RenderBundleEncoderDescriptor
{
};

class RenderPipeline;
class BindGroup;
class Buffer;
class JIPU_EXPORT RenderBundleEncoder
{
public:
    virtual ~RenderBundleEncoder() = default;

    RenderBundleEncoder(const RenderBundleEncoder&) = delete;
    RenderBundleEncoder& operator=(const RenderBundleEncoder&) = delete;

public:
public:
    virtual void setPipeline(RenderPipeline* pipeline) = 0;
    virtual void setBindGroup(uint32_t index, BindGroup* bindGroup, std::vector<uint32_t> dynamicOffset = {}) = 0;

    virtual void setVertexBuffer(uint32_t slot, Buffer* buffer) = 0;
    virtual void setIndexBuffer(Buffer* buffer, IndexFormat format) = 0;

    virtual void draw(uint32_t vertexCount,
                      uint32_t instanceCount,
                      uint32_t firstVertex,
                      uint32_t firstInstance) = 0;
    virtual void drawIndexed(uint32_t indexCount,
                             uint32_t instanceCount,
                             uint32_t indexOffset,
                             uint32_t vertexOffset,
                             uint32_t firstInstance) = 0;

    virtual std::unique_ptr<RenderBundle> finish(const RenderBundleDescriptor& descriptor) = 0;

protected:
    RenderBundleEncoder() = default;
};

} // namespace jipu