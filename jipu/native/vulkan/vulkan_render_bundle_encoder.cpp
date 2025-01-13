#include "vulkan_render_bundle_encoder.h"

#include "vulkan_device.h"
#include "vulkan_render_bundle.h"

namespace jipu
{

std::unique_ptr<RenderBundleEncoder> VulkanRenderBundleEncoder::create(VulkanDevice* device, const RenderBundleEncoderDescriptor& descriptor)
{
    return std::unique_ptr<RenderBundleEncoder>(new VulkanRenderBundleEncoder(device, descriptor));
}

VulkanRenderBundleEncoder::VulkanRenderBundleEncoder(VulkanDevice* device, const RenderBundleEncoderDescriptor& descriptor)
    : RenderBundleEncoder()
    , m_device(device)
    , m_descriptor(descriptor)
{
}

void VulkanRenderBundleEncoder::setPipeline(RenderPipeline* pipeline)
{
    SetRenderPipelineCommand command{
        { .type = CommandType::kSetRenderPipeline },
        .pipeline = pipeline
    };

    addCommand(std::make_unique<SetRenderPipelineCommand>(std::move(command)));
}

void VulkanRenderBundleEncoder::setBindGroup(uint32_t index, BindGroup* bindGroup, std::vector<uint32_t> dynamicOffset)
{
    SetBindGroupCommand command{
        { .type = CommandType::kSetRenderBindGroup },
        .index = index,
        .bindGroup = bindGroup,
        .dynamicOffset = dynamicOffset
    };

    addCommand(std::make_unique<SetBindGroupCommand>(std::move(command)));
}

void VulkanRenderBundleEncoder::setVertexBuffer(uint32_t slot, Buffer* buffer)
{
    SetVertexBufferCommand command{
        { .type = CommandType::kSetVertexBuffer },
        .slot = slot,
        .buffer = buffer
    };

    addCommand(std::make_unique<SetVertexBufferCommand>(std::move(command)));
}

void VulkanRenderBundleEncoder::setIndexBuffer(Buffer* buffer, IndexFormat format)
{
    SetIndexBufferCommand command{
        { .type = CommandType::kSetIndexBuffer },
        .buffer = buffer,
        .format = format
    };

    addCommand(std::make_unique<SetIndexBufferCommand>(std::move(command)));
}

void VulkanRenderBundleEncoder::draw(uint32_t vertexCount,
                                     uint32_t instanceCount,
                                     uint32_t firstVertex,
                                     uint32_t firstInstance)
{
    DrawCommand command{
        { .type = CommandType::kDraw },
        .vertexCount = vertexCount,
        .instanceCount = instanceCount,
        .firstVertex = firstVertex,
        .firstInstance = firstInstance
    };

    addCommand(std::make_unique<DrawCommand>(std::move(command)));
}

void VulkanRenderBundleEncoder::drawIndexed(uint32_t indexCount,
                                            uint32_t instanceCount,
                                            uint32_t indexOffset,
                                            uint32_t vertexOffset,
                                            uint32_t firstInstance)
{
    DrawIndexedCommand command{
        { .type = CommandType::kDrawIndexed },
        .indexCount = indexCount,
        .instanceCount = instanceCount,
        .indexOffset = indexOffset,
        .vertexOffset = vertexOffset,
        .firstInstance = firstInstance
    };

    addCommand(std::make_unique<DrawIndexedCommand>(std::move(command)));
}

std::unique_ptr<RenderBundle> VulkanRenderBundleEncoder::finish(const RenderBundleDescriptor& descriptor)
{
    return VulkanRenderBundle::create(this, descriptor);
}

VulkanDevice* VulkanRenderBundleEncoder::getDevice() const
{
    return m_device;
}

void VulkanRenderBundleEncoder::addCommand(std::unique_ptr<Command> command)
{
    switch (command->type)
    {
    case CommandType::kSetRenderPipeline:
        // nothing to do
        break;
    case CommandType::kSetVertexBuffer:
        // nothing to do
        break;
    case CommandType::kSetIndexBuffer:
        // nothing to do
        break;
    case CommandType::kDraw:
        // nothing to do
        break;
    case CommandType::kDrawIndexed:
        // nothing to do
        break;
    case CommandType::kDrawIndirect:
        // nothing to do
        break;
    case CommandType::kDrawIndexedIndirect:
        // nothing to do
        break;
    case CommandType::kSetRenderBindGroup:
        // nothing to do
        break;
    default:
        throw std::runtime_error("Unknown command type.");
        break;
    }

    m_commands.push_back(std::move(command));
}

CommandEncodingResult VulkanRenderBundleEncoder::extractResult()
{
    return CommandEncodingResult{
        .commands = std::move(m_commands)
    };
}

} // namespace jipu