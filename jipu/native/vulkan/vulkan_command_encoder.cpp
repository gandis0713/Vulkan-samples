#include "vulkan_command_encoder.h"

#include "vulkan_compute_pass_encoder.h"
#include "vulkan_device.h"
#include "vulkan_render_pass_encoder.h"

namespace jipu
{

VulkanCommandEncoder::VulkanCommandEncoder(VulkanDevice* device, const CommandEncoderDescriptor& descriptor)
    : m_device(device)
{
}

std::unique_ptr<ComputePassEncoder> VulkanCommandEncoder::beginComputePass(const ComputePassEncoderDescriptor& descriptor)
{
    return std::make_unique<VulkanComputePassEncoder>(this, descriptor);
}

std::unique_ptr<RenderPassEncoder> VulkanCommandEncoder::beginRenderPass(const RenderPassEncoderDescriptor& descriptor)
{
    return std::make_unique<VulkanRenderPassEncoder>(this, descriptor);
}

std::unique_ptr<RenderPassEncoder> VulkanCommandEncoder::beginRenderPass(const VulkanRenderPassEncoderDescriptor& descriptor)
{
    // TODO: encode begin render pass command
    return std::make_unique<VulkanRenderPassEncoder>(this, descriptor);
}

void VulkanCommandEncoder::copyBufferToBuffer(const CopyBuffer& src, const CopyBuffer& dst, uint64_t size)
{
    CopyBufferToBufferCommand command{
        { .type = CommandType::kCopyBufferToBuffer },
        src,
        dst,
        size
    };

    m_commands.push_back(std::make_unique<CopyBufferToBufferCommand>(std::move(command)));
}

void VulkanCommandEncoder::copyBufferToTexture(const CopyTextureBuffer& buffer, const CopyTexture& texture, const Extent3D& extent)
{
    CopyBufferToTextureCommand command{
        { .type = CommandType::kCopyBufferToTexture },
        buffer,
        texture,
        extent
    };

    m_commands.push_back(std::make_unique<CopyBufferToTextureCommand>(std::move(command)));
}

void VulkanCommandEncoder::copyTextureToBuffer(const CopyTexture& texture, const CopyTextureBuffer& buffer, const Extent3D& extent)
{
    CopyTextureToBufferCommand command{
        { .type = CommandType::kCopyTextureToBuffer },
        texture,
        buffer,
        extent
    };

    m_commands.push_back(std::make_unique<CopyTextureToBufferCommand>(std::move(command)));
}

void VulkanCommandEncoder::copyTextureToTexture(const CopyTexture& src, const CopyTexture& dst, const Extent3D& extent)
{
    CopyTextureToTextureCommand command{
        { .type = CommandType::kCopyTextureToTexture },
        src,
        dst,
        extent
    };

    m_commands.push_back(std::make_unique<CopyTextureToTextureCommand>(std::move(command)));
}

void VulkanCommandEncoder::resolveQuerySet(QuerySet* querySet,
                                           uint32_t firstQuery,
                                           uint32_t queryCount,
                                           Buffer* destination,
                                           uint64_t destinationOffset)
{

    ResolveQuerySetCommand command{
        { .type = CommandType::kResolveQuerySet },
        querySet,
        firstQuery,
        // queryCount,
        destination,
        destinationOffset
    };

    m_commands.push_back(std::make_unique<ResolveQuerySetCommand>(std::move(command)));
}

std::unique_ptr<CommandBuffer> VulkanCommandEncoder::finish(const CommandBufferDescriptor& descriptor)
{
    return std::make_unique<VulkanCommandBuffer>(this, descriptor);
}

void VulkanCommandEncoder::addCommand(std::unique_ptr<Command> command)
{
    switch (command->type)
    {
    case CommandType::kBeginComputePass:
        m_commandResourceTracker.beginComputePass(reinterpret_cast<BeginComputePassCommand*>(command.get()));
        break;
    case CommandType::kEndComputePass:
        m_commandResourceTracker.endComputePass(reinterpret_cast<EndComputePassCommand*>(command.get()));
        break;
    case CommandType::kSetComputePipeline:
        // setComputePipeline(reinterpret_cast<SetComputePipelineCommand*>(command.get()));
        break;
    case CommandType::kDispatch:
        // dispatch(reinterpret_cast<DispatchCommand*>(command.get()));
        break;
    case CommandType::kDispatchIndirect:
        // dispatchIndirect(reinterpret_cast<DispatchIndirectCommand*>(command.get()));
        break;
    case CommandType::kBeginRenderPass:
        m_commandResourceTracker.beginRenderPass(reinterpret_cast<BeginRenderPassCommand*>(command.get()));
        break;
    case CommandType::kSetRenderPipeline:
        // setRenderPipeline(reinterpret_cast<SetRenderPipelineCommand*>(command.get()));
        break;
    case CommandType::kSetVertexBuffer:
        m_commandResourceTracker.setVertexBuffer(reinterpret_cast<SetVertexBufferCommand*>(command.get()));
        break;
    case CommandType::kSetIndexBuffer:
        m_commandResourceTracker.setIndexBuffer(reinterpret_cast<SetIndexBufferCommand*>(command.get()));
        break;
    case CommandType::kSetViewport:
        // setViewport(reinterpret_cast<SetViewportCommand*>(command.get()));
        break;
    case CommandType::kSetScissor:
        // setScissor(reinterpret_cast<SetScissorCommand*>(command.get()));
        break;
    case CommandType::kSetBlendConstant:
        // setBlendConstant(reinterpret_cast<SetBlendConstantCommand*>(command.get()));
        break;
    case CommandType::kDraw:
        // draw(reinterpret_cast<DrawCommand*>(command.get()));
        break;
    case CommandType::kDrawIndexed:
        // drawIndexed(reinterpret_cast<DrawIndexedCommand*>(command.get()));
        break;
    case CommandType::kDrawIndirect:
        // TODO: draw indirect
        break;
    case CommandType::kDrawIndexedIndirect:
        // TODO: draw indexed indirect
        break;
    case CommandType::kBeginOcclusionQuery:
        // beginOcclusionQuery(reinterpret_cast<BeginOcclusionQueryCommand*>(command.get()));
        break;
    case CommandType::kEndOcclusionQuery:
        // endOcclusionQuery(reinterpret_cast<EndOcclusionQueryCommand*>(command.get()));
        break;
    case CommandType::kEndRenderPass:
        m_commandResourceTracker.endRenderPass(reinterpret_cast<EndRenderPassCommand*>(command.get()));
        break;
    case CommandType::kSetComputeBindGroup:
        m_commandResourceTracker.setComputeBindGroup(reinterpret_cast<SetBindGroupCommand*>(command.get()));
        break;
    case CommandType::kSetRenderBindGroup:
        m_commandResourceTracker.setRenderBindGroup(reinterpret_cast<SetBindGroupCommand*>(command.get()));
        break;
    case CommandType::kClearBuffer:
        // TODO: clear buffer
        break;
    case CommandType::kCopyBufferToBuffer:
        // copyBufferToBuffer(reinterpret_cast<CopyBufferToBufferCommand*>(command.get()));
        break;
    case CommandType::kCopyBufferToTexture:
        // copyBufferToTexture(reinterpret_cast<CopyBufferToTextureCommand*>(command.get()));
        break;
    case CommandType::kCopyTextureToBuffer:
        // copyTextureToBuffer(reinterpret_cast<CopyTextureToBufferCommand*>(command.get()));
        break;
    case CommandType::kCopyTextureToTexture:
        // copyTextureToTexture(reinterpret_cast<CopyTextureToTextureCommand*>(command.get()));
        break;
    case CommandType::kResolveQuerySet:
        // resolveQuerySet(reinterpret_cast<ResolveQuerySetCommand*>(command.get()));
        break;
    case CommandType::kWriteTimestamp:
        // TODO: write timestamp
        break;
    default:
        throw std::runtime_error("Unknown command type.");
        break;
    }

    m_commands.push_back(std::move(command));
}

VulkanDevice* VulkanCommandEncoder::getDevice() const
{
    return m_device;
}

CommandEncodingResult VulkanCommandEncoder::finish()
{
    return CommandEncodingResult{
        .commands = std::move(m_commands),
        .resourceTrackingResult = m_commandResourceTracker.finish()
    };
}

} // namespace jipu