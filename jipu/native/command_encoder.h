#pragma once

#include "command_buffer.h"
#include "compute_pass_encoder.h"
#include "export.h"
#include "render_pass_encoder.h"
#include "texture.h"
#include <vector>

namespace jipu
{

class Pipeline;
class Buffer;
class CommandBuffer;
class TextureView;
class RenderPassEncoder;

struct BlitBuffer
{
    Buffer* buffer = nullptr;
    uint32_t offset = 0;
};

struct BlitTextureBuffer
{
    Buffer* buffer = nullptr;
    uint32_t offset = 0;
    uint32_t bytesPerRow = 0;
    uint32_t rowsPerTexture = 0;
};

struct BlitTexture
{
    Texture* texture = nullptr;
    TextureAspectFlags aspect = TextureAspectFlagBits::kUndefined;
};

struct CommandEncoderDescriptor
{
};

class JIPU_EXPORT CommandEncoder
{
public:
    virtual ~CommandEncoder() = default;

    virtual std::unique_ptr<ComputePassEncoder> beginComputePass(const ComputePassEncoderDescriptor& descriptor) = 0;
    virtual std::unique_ptr<RenderPassEncoder> beginRenderPass(const RenderPassEncoderDescriptor& descriptor) = 0;

    virtual void copyBufferToBuffer(const BlitBuffer& src,
                                    const BlitBuffer& dst,
                                    uint64_t size) = 0;
    virtual void copyBufferToTexture(const BlitTextureBuffer& buffer,
                                     const BlitTexture& texture,
                                     const Extent3D& extent) = 0;
    virtual void copyTextureToBuffer(const BlitTexture& texture,
                                     const BlitTextureBuffer& buffer,
                                     const Extent3D& extent) = 0;
    virtual void copyTextureToTexture(const BlitTexture& src,
                                      const BlitTexture& dst,
                                      const Extent3D& extent) = 0;
    virtual void resolveQuerySet(QuerySet* querySet,
                                 uint32_t firstQuery,
                                 uint32_t queryCount,
                                 Buffer* destination,
                                 uint64_t destinationOffset) = 0;

    virtual std::unique_ptr<CommandBuffer> finish(const CommandBufferDescriptor& descriptor) = 0;
};

} // namespace jipu