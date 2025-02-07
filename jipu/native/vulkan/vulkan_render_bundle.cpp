#include "vulkan_render_bundle.h"

#include "jipu/common/hash.h"
#include "vulkan_bind_group.h"
#include "vulkan_buffer.h"
#include "vulkan_command_pool.h"
#include "vulkan_device.h"
#include "vulkan_render_bundle_encoder.h"

namespace jipu
{

std::unique_ptr<RenderBundle> VulkanRenderBundle::create(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor)
{
    return std::unique_ptr<RenderBundle>(new VulkanRenderBundle(renderBundleEncoder, descriptor));
}

VulkanRenderBundle::VulkanRenderBundle(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor)
    : RenderBundle()
    , m_device(renderBundleEncoder->getDevice())
    , m_descriptor(descriptor)
    , m_commandEncodingResult(renderBundleEncoder->extractResult())
{
}

const std::vector<std::unique_ptr<Command>>& VulkanRenderBundle::getCommands() const
{
    return m_commandEncodingResult.commands;
}

VkCommandBuffer VulkanRenderBundle::getCommandBuffer(const VulkanCommandBufferInheritanceInfo& info)
{
    auto it = m_commandBuffers.find(info);
    if (it != m_commandBuffers.end())
    {
        return it->second;
    }

    beginRecord(info);

    setViewport(); // TODO: check if need to record scissor in secondary command buffer.
    setScissor();  // TODO: check if need to record scissor in secondary command buffer.

    const auto& commands = m_commandEncodingResult.commands;
    for (const auto& command : commands)
    {
        switch (command->type)
        {
        case CommandType::kSetRenderPipeline:
            setRenderPipeline(reinterpret_cast<SetRenderPipelineCommand*>(command.get()));
            break;
        case CommandType::kSetVertexBuffer:
            setVertexBuffer(reinterpret_cast<SetVertexBufferCommand*>(command.get()));
            break;
        case CommandType::kSetIndexBuffer:
            setIndexBuffer(reinterpret_cast<SetIndexBufferCommand*>(command.get()));
            break;
        case CommandType::kDraw:
            draw(reinterpret_cast<DrawCommand*>(command.get()));
            break;
        case CommandType::kDrawIndexed:
            drawIndexed(reinterpret_cast<DrawIndexedCommand*>(command.get()));
            break;
        case CommandType::kDrawIndirect:
            // TODO
            break;
        case CommandType::kDrawIndexedIndirect:
            // TODO
            break;
        case CommandType::kSetRenderBindGroup:
            setRenderBindGroup(reinterpret_cast<SetBindGroupCommand*>(command.get()));
            break;
        default:
            throw std::runtime_error("Unknown command type.");
            break;
        }
    }

    endRecord();

    it = m_commandBuffers.find(info);
    if (it != m_commandBuffers.end())
    {
        return it->second;
    }

    throw std::runtime_error("Failed to record render bundle command buffer.");
    return VK_NULL_HANDLE;
}

void VulkanRenderBundle::beginRecord(const VulkanCommandBufferInheritanceInfo& info)
{
    m_recordingContext.inheritanceInfo = info;
    m_recordingContext.commandBuffer = m_device->getCommandPool()->create(VulkanCommandBufferDescriptor{ .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY });

    VkCommandBufferInheritanceInfo inheritanceInfo{};
    inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass = m_recordingContext.inheritanceInfo.renderPass;
    inheritanceInfo.subpass = m_recordingContext.inheritanceInfo.subpass;
    inheritanceInfo.framebuffer = m_recordingContext.inheritanceInfo.framebuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;

    m_device->vkAPI.BeginCommandBuffer(m_recordingContext.commandBuffer, &beginInfo);
}

void VulkanRenderBundle::setRenderPipeline(SetRenderPipelineCommand* command)
{
    m_recordingContext.renderPipeline = downcast(command->pipeline);
    m_device->vkAPI.CmdBindPipeline(m_recordingContext.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_recordingContext.renderPipeline->getVkPipeline());
}

void VulkanRenderBundle::setRenderBindGroup(SetBindGroupCommand* command)
{
    auto vulkanBindGroup = downcast(command->bindGroup);
    VkDescriptorSet descriptorSet = vulkanBindGroup->getVkDescriptorSet();

    const VulkanAPI& vkAPI = m_device->vkAPI;
    vkAPI.CmdBindDescriptorSets(m_recordingContext.commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_recordingContext.renderPipeline->getVkPipelineLayout(),
                                command->index,
                                1,
                                &descriptorSet,
                                static_cast<uint32_t>(command->dynamicOffset.size()),
                                command->dynamicOffset.data());
}

void VulkanRenderBundle::setVertexBuffer(SetVertexBufferCommand* command)
{
    auto slot = command->slot;
    auto buffer = command->buffer;

    auto vulkanBuffer = downcast(buffer);
    VkBuffer vertexBuffers[] = { vulkanBuffer->getVkBuffer() };
    VkDeviceSize offsets[] = { 0 };
    m_device->vkAPI.CmdBindVertexBuffers(m_recordingContext.commandBuffer, slot, 1, vertexBuffers, offsets);
}

void VulkanRenderBundle::setIndexBuffer(SetIndexBufferCommand* command)
{
    auto buffer = command->buffer;
    auto format = command->format;

    auto vulkanBuffer = downcast(buffer);
    m_device->vkAPI.CmdBindIndexBuffer(m_recordingContext.commandBuffer, vulkanBuffer->getVkBuffer(), 0, ToVkIndexType(format));
}

void VulkanRenderBundle::setViewport()
{
    float width = 800.f;  // TODO
    float height = 600.f; // TODO

    VkViewport viewport = {
        .x = 0,
        .y = height,
        .width = width,
        .height = -height,
        .minDepth = 0,
        .maxDepth = 1,
    };

    m_device->vkAPI.CmdSetViewport(m_recordingContext.commandBuffer, 0, 1, &viewport);
}

void VulkanRenderBundle::setScissor()
{
    float width = 800.f;  // TODO
    float height = 600.f; // TODO

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) },
    };

    m_device->vkAPI.CmdSetScissor(m_recordingContext.commandBuffer, 0, 1, &scissor);
}

void VulkanRenderBundle::draw(DrawCommand* command)
{
    m_device->vkAPI.CmdDraw(m_recordingContext.commandBuffer, command->vertexCount, command->instanceCount, command->firstVertex, command->firstInstance);
}

void VulkanRenderBundle::drawIndexed(DrawIndexedCommand* command)
{
    m_device->vkAPI.CmdDrawIndexed(m_recordingContext.commandBuffer, command->indexCount, command->instanceCount, command->indexOffset, command->vertexOffset, command->firstInstance);
}

void VulkanRenderBundle::endRecord()
{
    m_device->vkAPI.EndCommandBuffer(m_recordingContext.commandBuffer);

    m_commandBuffers.insert({ m_recordingContext.inheritanceInfo, m_recordingContext.commandBuffer });
}

// VulkanRenderBundle secondary command buffer

size_t VulkanRenderBundle::Functor::operator()(const VulkanCommandBufferInheritanceInfo& info) const
{
    size_t hash = 0;

    combineHash(hash, info.subpass);

    if (info.renderPass)
        combineHash(hash, info.renderPass);

    if (info.framebuffer)
        combineHash(hash, info.framebuffer);

    return hash;
}

bool VulkanRenderBundle::Functor::operator()(const VulkanCommandBufferInheritanceInfo& lhs,
                                             const VulkanCommandBufferInheritanceInfo& rhs) const
{
    return lhs.renderPass == rhs.renderPass &&
           lhs.subpass == rhs.subpass &&
           lhs.framebuffer == rhs.framebuffer;
}

} // namespace jipu