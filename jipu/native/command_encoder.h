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

struct CopyBuffer
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
};

struct CopyTextureBuffer
{
    Buffer* buffer = nullptr;
    uint64_t offset = 0;
    uint32_t bytesPerRow = 0;
    uint32_t rowsPerTexture = 0;
};

struct CopyTexture
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

    CommandEncoder(const CommandEncoder&) = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    virtual std::unique_ptr<ComputePassEncoder> beginComputePass(const ComputePassEncoderDescriptor& descriptor) = 0;
    virtual std::unique_ptr<RenderPassEncoder> beginRenderPass(const RenderPassEncoderDescriptor& descriptor) = 0;

    virtual void copyBufferToBuffer(const CopyBuffer& src,
                                    const CopyBuffer& dst,
                                    uint64_t size) = 0;
    virtual void copyBufferToTexture(const CopyTextureBuffer& buffer,
                                     const CopyTexture& texture,
                                     const Extent3D& extent) = 0;
    virtual void copyTextureToBuffer(const CopyTexture& texture,
                                     const CopyTextureBuffer& buffer,
                                     const Extent3D& extent) = 0;
    virtual void copyTextureToTexture(const CopyTexture& src,
                                      const CopyTexture& dst,
                                      const Extent3D& extent) = 0;
    virtual void resolveQuerySet(QuerySet* querySet,
                                 uint32_t firstQuery,
                                 uint32_t queryCount,
                                 Buffer* destination,
                                 uint64_t destinationOffset) = 0;

    virtual std::unique_ptr<CommandBuffer> finish(const CommandBufferDescriptor& descriptor) = 0;

protected:
    CommandEncoder() = default;
};

} // namespace jipu