#pragma once

#include "bind_group.h"
#include "buffer.h"
#include "command_encoder.h"
#include "pipeline.h"
#include "query_set.h"
#include "render_pass_encoder.h"
#include "texture_view.h"

#include "vulkan_api.h"

namespace jipu
{

class VulkanRenderPass;
class VulkanFramebuffer;

enum class CommandType
{
    kNone = 0,

    kBeginComputePass,
    kSetComputePipeline,
    kSetComputeBindGroup,
    kDispatch,
    kDispatchIndirect,
    kEndComputePass,

    kBeginRenderPass,
    kSetRenderPipeline,
    kSetRenderBindGroup,
    kSetIndexBuffer,
    kSetVertexBuffer,
    kSetViewport,
    kSetScissor,
    kSetBlendConstant,
    kDraw,
    kDrawIndexed,
    kDrawIndirect,
    kDrawIndexedIndirect,
    kExecuteBundle,
    kEndRenderPass,

    kClearBuffer,

    kCopyBufferToBuffer,
    kCopyBufferToTexture,
    kCopyTextureToBuffer,
    kCopyTextureToTexture,

    kBeginOcclusionQuery,
    kEndOcclusionQuery,
    kResolveQuerySet,

    kWriteTimestamp,
};

struct Command
{
    CommandType type;
};

struct BeginComputePassCommand : public Command
{
};

struct EndComputePassCommand : public Command
{
};

struct BeginRenderPassCommand : public Command
{
    std::weak_ptr<VulkanRenderPass> renderPass{};
    std::weak_ptr<VulkanFramebuffer> framebuffer{};
    VkRect2D renderArea{};
    std::vector<VkClearValue> clearValues{};

    // TODO: convert timestampWrites for vulkan.
    QuerySet* occlusionQuerySet = nullptr;
    RenderPassTimestampWrites timestampWrites{};
};

struct EndRenderPassCommand : public Command
{
};

struct ClearBufferCommand : public Command
{
};

struct CopyBufferToBufferCommand : public Command
{
    CopyBuffer src{};
    CopyBuffer dst{};
    uint64_t size = 0;
};

struct CopyBufferToTextureCommand : public Command
{
    CopyTextureBuffer buffer{};
    CopyTexture texture{};
    Extent3D extent{};
};

struct CopyTextureToBufferCommand : public Command
{
    CopyTexture texture{};
    CopyTextureBuffer buffer{};
    Extent3D extent{};
};

struct CopyTextureToTextureCommand : public Command
{
    CopyTexture src{};
    CopyTexture dst{};
    Extent3D extent{};
};

struct DispatchCommand : public Command
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
};

struct DispatchIndirectCommand : public Command
{
};

struct DrawCommand : public Command
{
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstVertex = 0;
    uint32_t firstInstance = 0;
};

struct DrawIndexedCommand : public Command
{
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t indexOffset = 0;
    uint32_t vertexOffset = 0;
    uint32_t firstInstance = 0;
};

struct ExecuteBundleCommand : public Command
{
    std::vector<RenderBundle*> renderBundles{};
};

struct DrawIndirectCommand : public Command
{
};

struct DrawIndexedIndirectCommand : public Command
{
};

struct BeginOcclusionQueryCommand : public Command
{
    QuerySet* querySet = nullptr;
    uint32_t queryIndex = 0;
};

struct EndOcclusionQueryCommand : public Command
{
    QuerySet* querySet = nullptr;
};

struct ResolveQuerySetCommand : public Command
{
    QuerySet* querySet = nullptr;
    uint32_t firstQuery = 0;
    // uint32_t queryCount = 0;
    Buffer* destination = nullptr;
    uint64_t destinationOffset = 0;
};

struct SetComputePipelineCommand : public Command
{
    ComputePipeline* pipeline = nullptr;
};

struct SetRenderPipelineCommand : public Command
{
    RenderPipeline* pipeline = nullptr;
};

struct SetViewportCommand : public Command
{
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
    float minDepth = 0;
    float maxDepth = 0;
};

struct SetScissorCommand : public Command
{
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
};

struct SetBlendConstantCommand : public Command
{
    Color color{};
};

struct SetBindGroupCommand : public Command
{
    uint32_t index = 0;
    BindGroup* bindGroup = nullptr;
    std::vector<uint32_t> dynamicOffset{};
};

struct SetIndexBufferCommand : public Command
{
    Buffer* buffer = nullptr;
    IndexFormat format = IndexFormat::kUndefined;
};

struct SetVertexBufferCommand : public Command
{
    uint32_t slot = 0;
    Buffer* buffer = nullptr;
};

struct WriteTimestampCommand : public Command
{
};

} // namespace jipu