#include "vulkan_command_recorder.h"

#include "vulkan_bind_group.h"
#include "vulkan_buffer.h"
#include "vulkan_command.h"
#include "vulkan_command_buffer.h"
#include "vulkan_command_pool.h"
#include "vulkan_compute_pass_encoder.h"
#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_pipeline_layout.h"
#include "vulkan_query_set.h"
#include "vulkan_render_bundle.h"
#include "vulkan_render_pass_encoder.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanCommandRecorder::VulkanCommandRecorder(VulkanCommandBuffer* commandBuffer, VulkanCommandRecorderDescriptor descriptor)
    : m_commandBuffer(commandBuffer)
    , m_descriptor(std::move(descriptor))
    , m_commandResourceSyncronizer(this, VulkanCommandResourceSynchronizerDescriptor{ .operationResourceInfos = m_descriptor.commandEncodingResult.resourceTrackingResult.operationResourceInfos })
{
}

VulkanCommandRecorder::~VulkanCommandRecorder()
{
    // do not destroy or release the command buffer. because the command buffer is returned as result.
}

VulkanCommandRecordResult VulkanCommandRecorder::record()
{
    beginRecord();

    for (const auto& command : m_descriptor.commandEncodingResult.commands)
    {
        switch (command->type)
        {
        case CommandType::kBeginComputePass:
            beginComputePass(reinterpret_cast<BeginComputePassCommand*>(command.get()));
            break;
        case CommandType::kEndComputePass:
            endComputePass(reinterpret_cast<EndComputePassCommand*>(command.get()));
            break;
        case CommandType::kSetComputePipeline:
            setComputePipeline(reinterpret_cast<SetComputePipelineCommand*>(command.get()));
            break;
        case CommandType::kDispatch:
            dispatch(reinterpret_cast<DispatchCommand*>(command.get()));
            break;
        case CommandType::kDispatchIndirect:
            dispatchIndirect(reinterpret_cast<DispatchIndirectCommand*>(command.get()));
            break;
        case CommandType::kBeginRenderPass:
            beginRenderPass(reinterpret_cast<BeginRenderPassCommand*>(command.get()));
            break;
        case CommandType::kSetRenderPipeline:
            setRenderPipeline(reinterpret_cast<SetRenderPipelineCommand*>(command.get()));
            break;
        case CommandType::kSetVertexBuffer:
            setVertexBuffer(reinterpret_cast<SetVertexBufferCommand*>(command.get()));
            break;
        case CommandType::kSetIndexBuffer:
            setIndexBuffer(reinterpret_cast<SetIndexBufferCommand*>(command.get()));
            break;
        case CommandType::kSetViewport:
            setViewport(reinterpret_cast<SetViewportCommand*>(command.get()));
            break;
        case CommandType::kSetScissor:
            setScissor(reinterpret_cast<SetScissorCommand*>(command.get()));
            break;
        case CommandType::kSetBlendConstant:
            setBlendConstant(reinterpret_cast<SetBlendConstantCommand*>(command.get()));
            break;
        case CommandType::kDraw:
            draw(reinterpret_cast<DrawCommand*>(command.get()));
            break;
        case CommandType::kDrawIndexed:
            drawIndexed(reinterpret_cast<DrawIndexedCommand*>(command.get()));
            break;
        case CommandType::kDrawIndirect:
            // TODO: draw indirect
            break;
        case CommandType::kDrawIndexedIndirect:
            // TODO: draw indexed indirect
            break;
        case CommandType::kBeginOcclusionQuery:
            beginOcclusionQuery(reinterpret_cast<BeginOcclusionQueryCommand*>(command.get()));
            break;
        case CommandType::kEndOcclusionQuery:
            endOcclusionQuery(reinterpret_cast<EndOcclusionQueryCommand*>(command.get()));
            break;
        case CommandType::kEndRenderPass:
            endRenderPass(reinterpret_cast<EndRenderPassCommand*>(command.get()));
            break;
        case CommandType::kSetComputeBindGroup:
            setComputeBindGroup(reinterpret_cast<SetBindGroupCommand*>(command.get()));
            break;
        case CommandType::kSetRenderBindGroup:
            setRenderBindGroup(reinterpret_cast<SetBindGroupCommand*>(command.get()));
            break;
        case CommandType::kClearBuffer:
            // TODO: clear buffer
            break;
        case CommandType::kCopyBufferToBuffer:
            copyBufferToBuffer(reinterpret_cast<CopyBufferToBufferCommand*>(command.get()));
            break;
        case CommandType::kCopyBufferToTexture:
            copyBufferToTexture(reinterpret_cast<CopyBufferToTextureCommand*>(command.get()));
            break;
        case CommandType::kCopyTextureToBuffer:
            copyTextureToBuffer(reinterpret_cast<CopyTextureToBufferCommand*>(command.get()));
            break;
        case CommandType::kCopyTextureToTexture:
            copyTextureToTexture(reinterpret_cast<CopyTextureToTextureCommand*>(command.get()));
            break;
        case CommandType::kResolveQuerySet:
            resolveQuerySet(reinterpret_cast<ResolveQuerySetCommand*>(command.get()));
            break;
        case CommandType::kWriteTimestamp:
            // TODO: write timestamp
            break;
        case CommandType::kExecuteBundle:
            executeBundle(reinterpret_cast<ExecuteBundleCommand*>(command.get()));
            break;
        default:
            throw std::runtime_error("Unknown command type.");
            break;
        }
    }

    return endRecord();
}

void VulkanCommandRecorder::beginRecord()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr; // Optional

    if (m_commandBuffer->getDevice()->vkAPI.BeginCommandBuffer(m_commandBuffer->getVkCommandBuffer(), &commandBufferBeginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin command buffer.");
    }
}

VulkanCommandRecordResult VulkanCommandRecorder::endRecord()
{
    if (m_commandBuffer->getDevice()->vkAPI.EndCommandBuffer(m_commandBuffer->getVkCommandBuffer()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to end command buffer.");
    }

    return VulkanCommandRecordResult{
        .commands = std::move(m_descriptor.commandEncodingResult.commands),
        .resourceSyncResult = m_commandResourceSyncronizer.finish()
    };
}

void VulkanCommandRecorder::beginComputePass(BeginComputePassCommand* command)
{
    m_commandResourceSyncronizer.beginComputePass(command);
}

void VulkanCommandRecorder::setComputePipeline(SetComputePipelineCommand* command)
{
    m_commandResourceSyncronizer.setComputePipeline(command);

    m_computePipeline = downcast(command->pipeline);

    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    vkAPI.CmdBindPipeline(m_commandBuffer->getVkCommandBuffer(),
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          m_computePipeline->getVkPipeline());
}

void VulkanCommandRecorder::setComputeBindGroup(SetBindGroupCommand* command)
{
    if (!m_computePipeline)
        throw std::runtime_error("The pipeline is null");

    m_commandResourceSyncronizer.setComputeBindGroup(command);

    auto vulkanBindGroup = downcast(command->bindGroup);

    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    VkDescriptorSet descriptorSet = vulkanBindGroup->getVkDescriptorSet();

    vkAPI.CmdBindDescriptorSets(m_commandBuffer->getVkCommandBuffer(),
                                VK_PIPELINE_BIND_POINT_COMPUTE,
                                m_computePipeline->getVkPipelineLayout(),
                                command->index,
                                1,
                                &descriptorSet,
                                static_cast<uint32_t>(command->dynamicOffset.size()),
                                command->dynamicOffset.data());
}

void VulkanCommandRecorder::dispatch(DispatchCommand* command)
{
    m_commandResourceSyncronizer.dispatch(command);

    auto x = command->x;
    auto y = command->y;
    auto z = command->z;

    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    vkAPI.CmdDispatch(m_commandBuffer->getVkCommandBuffer(), x, y, z);
}

void VulkanCommandRecorder::dispatchIndirect(DispatchIndirectCommand* command)
{
    m_commandResourceSyncronizer.dispatchIndirect(command);

    // TODO: dispatch indirect
}

void VulkanCommandRecorder::endComputePass(EndComputePassCommand* command)
{
    m_commandResourceSyncronizer.endComputePass(command);

    // do nothing.
}

void VulkanCommandRecorder::beginRenderPass(BeginRenderPassCommand* command)
{
    m_commandResourceSyncronizer.beginRenderPass(command);

    const auto& vkAPI = m_commandBuffer->getDevice()->vkAPI;
    // if (command->timestampWrites.querySet)
    // {
    //     auto vulkanQuerySet = downcast(command->timestampWrites.querySet);
    //     vkAPI.CmdResetQueryPool(m_commandBuffer,
    //                             vulkanQuerySet->getVkQueryPool(),
    //                             0,
    //                             vulkanQuerySet->getCount());
    //     vkAPI.CmdWriteTimestamp(m_commandBuffer,
    //                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //                             vulkanQuerySet->getVkQueryPool(),
    //                             command->timestampWrites.beginQueryIndex);
    // }

    auto renderPass = command->renderPass.lock();
    if (!renderPass)
        throw std::runtime_error("The render pass is null for begin render pass command.");

    auto framebuffer = command->framebuffer.lock();
    if (!framebuffer)
        throw std::runtime_error("The framebuffer is null for begin render pass command.");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass->getVkRenderPass();
    renderPassInfo.framebuffer = framebuffer->getVkFrameBuffer();
    renderPassInfo.renderArea = command->renderArea;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(command->clearValues.size());
    renderPassInfo.pClearValues = command->clearValues.data();

    vkAPI.CmdBeginRenderPass(m_commandBuffer->getVkCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandRecorder::setRenderPipeline(SetRenderPipelineCommand* command)
{
    m_commandResourceSyncronizer.setRenderPipeline(command);

    m_renderPipeline = downcast(command->pipeline);

    m_commandBuffer->getDevice()->vkAPI.CmdBindPipeline(m_commandBuffer->getVkCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderPipeline->getVkPipeline());
}

void VulkanCommandRecorder::setRenderBindGroup(SetBindGroupCommand* command)
{
    if (!m_renderPipeline)
        throw std::runtime_error("The pipeline is null");

    m_commandResourceSyncronizer.setRenderBindGroup(command);

    auto vulkanBindGroup = downcast(command->bindGroup);

    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    VkDescriptorSet descriptorSet = vulkanBindGroup->getVkDescriptorSet();

    vkAPI.CmdBindDescriptorSets(m_commandBuffer->getVkCommandBuffer(),
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_renderPipeline->getVkPipelineLayout(),
                                command->index,
                                1,
                                &descriptorSet,
                                static_cast<uint32_t>(command->dynamicOffset.size()),
                                command->dynamicOffset.data());
}

void VulkanCommandRecorder::setVertexBuffer(SetVertexBufferCommand* command)
{
    m_commandResourceSyncronizer.setVertexBuffer(command);

    auto slot = command->slot;
    auto buffer = command->buffer;

    auto vulkanBuffer = downcast(buffer);
    VkBuffer vertexBuffers[] = { vulkanBuffer->getVkBuffer() };
    VkDeviceSize offsets[] = { 0 };
    m_commandBuffer->getDevice()->vkAPI.CmdBindVertexBuffers(m_commandBuffer->getVkCommandBuffer(), slot, 1, vertexBuffers, offsets);
}

void VulkanCommandRecorder::setIndexBuffer(SetIndexBufferCommand* command)
{
    m_commandResourceSyncronizer.setIndexBuffer(command);

    auto buffer = command->buffer;
    auto format = command->format;

    auto vulkanBuffer = downcast(buffer);
    m_commandBuffer->getDevice()->vkAPI.CmdBindIndexBuffer(m_commandBuffer->getVkCommandBuffer(), vulkanBuffer->getVkBuffer(), 0, ToVkIndexType(format));
}

void VulkanCommandRecorder::setViewport(SetViewportCommand* command)
{
    m_commandResourceSyncronizer.setViewport(command);

    auto x = command->x;
    auto y = command->y;
    auto width = command->width;
    auto height = command->height;
    auto minDepth = command->minDepth;
    auto maxDepth = command->maxDepth;

    VkViewport viewport{ x,
                         y + height,
                         width,
                         -height,
                         minDepth,
                         maxDepth };
    m_commandBuffer->getDevice()->vkAPI.CmdSetViewport(m_commandBuffer->getVkCommandBuffer(), 0, 1, &viewport);
}

void VulkanCommandRecorder::setScissor(SetScissorCommand* command)
{
    m_commandResourceSyncronizer.setScissor(command);

    auto x = command->x;
    auto y = command->y;
    auto width = command->width;
    auto height = command->height;

    VkRect2D scissorRect{};
    scissorRect.offset.x = x;
    scissorRect.offset.y = y;
    scissorRect.extent.width = width;
    scissorRect.extent.height = height;

    m_commandBuffer->getDevice()->vkAPI.CmdSetScissor(m_commandBuffer->getVkCommandBuffer(), 0, 1, &scissorRect);
}

void VulkanCommandRecorder::setBlendConstant(SetBlendConstantCommand* command)
{
    m_commandResourceSyncronizer.setBlendConstant(command);

    auto color = command->color;

    float blendConstants[4] = { static_cast<float>(color.r),
                                static_cast<float>(color.g),
                                static_cast<float>(color.b),
                                static_cast<float>(color.a) };

    m_commandBuffer->getDevice()->vkAPI.CmdSetBlendConstants(m_commandBuffer->getVkCommandBuffer(), blendConstants);
}

void VulkanCommandRecorder::executeBundle(ExecuteBundleCommand* command)
{
    // do not call executeBundle in command resource synchronizer.
    // it is called in each command function.

    for (auto& renderBundle : command->renderBundles)
    {
        auto vulkanRenderBundle = downcast(renderBundle);
        const auto& commands = vulkanRenderBundle->getCommands();
        for (auto& command : commands)
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
    }
}

void VulkanCommandRecorder::draw(DrawCommand* command)
{
    m_commandResourceSyncronizer.draw(command);

    auto vertexCount = command->vertexCount;
    auto instanceCount = command->instanceCount;
    auto firstVertex = command->firstVertex;
    auto firstInstance = command->firstInstance;

    m_commandBuffer->getDevice()->vkAPI.CmdDraw(m_commandBuffer->getVkCommandBuffer(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandRecorder::drawIndexed(DrawIndexedCommand* command)
{
    m_commandResourceSyncronizer.drawIndexed(command);

    auto indexCount = command->indexCount;
    auto instanceCount = command->instanceCount;
    auto indexOffset = command->indexOffset;
    auto vertexOffset = command->vertexOffset;
    auto firstInstance = command->firstInstance;

    m_commandBuffer->getDevice()->vkAPI.CmdDrawIndexed(m_commandBuffer->getVkCommandBuffer(),
                                                       indexCount,
                                                       instanceCount,
                                                       indexOffset,
                                                       vertexOffset,
                                                       firstInstance);
}

void VulkanCommandRecorder::beginOcclusionQuery(BeginOcclusionQueryCommand* command)
{
    m_commandResourceSyncronizer.beginOcclusionQuery(command);

    auto queryIndex = command->queryIndex;
    auto querySet = command->querySet;
    auto vulkanOcclusionQuerySet = downcast(querySet);
    auto& vkAPI = m_commandBuffer->getDevice()->vkAPI;
    vkAPI.CmdBeginQuery(m_commandBuffer->getVkCommandBuffer(),
                        vulkanOcclusionQuerySet->getVkQueryPool(),
                        queryIndex,
                        0);
}

void VulkanCommandRecorder::endOcclusionQuery(EndOcclusionQueryCommand* command)
{
    m_commandResourceSyncronizer.endOcclusionQuery(command);

    auto querySet = command->querySet;
    auto vulkanOcclusionQuerySet = downcast(querySet);
    auto& vkAPI = m_commandBuffer->getDevice()->vkAPI;
    vkAPI.CmdEndQuery(m_commandBuffer->getVkCommandBuffer(),
                      vulkanOcclusionQuerySet->getVkQueryPool(),
                      0);
}

void VulkanCommandRecorder::endRenderPass(EndRenderPassCommand* command)
{
    m_commandResourceSyncronizer.endRenderPass(command);

    const auto& vkAPI = m_commandBuffer->getDevice()->vkAPI;
    vkAPI.CmdEndRenderPass(m_commandBuffer->getVkCommandBuffer());

    // if (m_descriptor.timestampWrites.querySet)
    // {
    //     auto vulkanQuerySet = downcast(m_descriptor.timestampWrites.querySet);
    //     vkAPI.CmdWriteTimestamp(m_commandBuffer,
    //                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    //                             vulkanQuerySet->getVkQueryPool(),
    //                             m_descriptor.timestampWrites.endQueryIndex);
    // }
}

void VulkanCommandRecorder::copyBufferToBuffer(CopyBufferToBufferCommand* command)
{
    m_commandResourceSyncronizer.copyBufferToBuffer(command);

    auto& src = command->src;
    auto& dst = command->dst;
    auto& size = command->size;

    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = src.offset;
    copyRegion.dstOffset = dst.offset;
    copyRegion.size = size;

    VkBuffer srcBuffer = downcast(src.buffer)->getVkBuffer();
    VkBuffer dstBuffer = downcast(dst.buffer)->getVkBuffer();

    vkAPI.CmdCopyBuffer(m_commandBuffer->getVkCommandBuffer(), srcBuffer, dstBuffer, 1, &copyRegion);
}

void VulkanCommandRecorder::copyBufferToTexture(CopyBufferToTextureCommand* command)
{
    m_commandResourceSyncronizer.copyBufferToTexture(command);

    auto& buffer = command->buffer;
    auto& texture = command->texture;
    auto& extent = command->extent;

    auto vulkanTexture = downcast(texture.texture);
    if (!(vulkanTexture->getUsage() & TextureUsageFlagBits::kCopyDst))
        throw std::runtime_error("The texture is not used for copy dst.");

    VkCommandBuffer commandBuffer = m_commandBuffer->getVkCommandBuffer();
    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    VkImageSubresourceRange range;
    range.aspectMask = ToVkImageAspectFlags(texture.aspect);
    range.baseMipLevel = texture.mipLevel;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    auto previousLayout = vulkanTexture->getCurrentLayout(texture.mipLevel);
    // change layout
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = previousLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkanTexture->getVkImage();
        barrier.subresourceRange = range;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        vulkanTexture->cmdPipelineBarrier(commandBuffer, srcStage, dstStage, barrier);
    }

    // copy buffer to texture
    auto vulkanBuffer = downcast(buffer.buffer);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = ToVkImageAspectFlags(texture.aspect);
    region.imageSubresource.mipLevel = texture.mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { .width = extent.width,
                           .height = extent.height,
                           .depth = extent.depth };

    vkAPI.CmdCopyBufferToImage(commandBuffer,
                               vulkanBuffer->getVkBuffer(),
                               vulkanTexture->getVkImage(),
                               // dstImageLayout must be
                               //   VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR
                               //   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                               //   VK_IMAGE_LAYOUT_GENERAL
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);

    // restore layout
    // TODO: restore layout if need to use the texture in the next command.
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        barrier.oldLayout = vulkanTexture->getCurrentLayout(texture.mipLevel);
        barrier.newLayout = previousLayout == VK_IMAGE_LAYOUT_UNDEFINED ? vulkanTexture->getFinalLayout() : previousLayout; // TODO: image layout
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkanTexture->getVkImage();
        barrier.subresourceRange = range;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // VK_PIPELINE_STAGE_NONE if synchronization2 is enabled.

        vulkanTexture->cmdPipelineBarrier(commandBuffer, srcStage, dstStage, barrier);
    }
}

void VulkanCommandRecorder::copyTextureToBuffer(CopyTextureToBufferCommand* command)
{
    m_commandResourceSyncronizer.copyTextureToBuffer(command);

    auto& texture = command->texture;
    auto& buffer = command->buffer;
    auto& extent = command->extent;

    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    // set pipeline barrier to change image layout
    auto vulkanTexture = downcast(texture.texture);

    VkImageSubresourceRange range{};
    range.aspectMask = ToVkImageAspectFlags(texture.aspect);
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    range.baseMipLevel = 0;
    range.levelCount = 1;

    auto srcImage = vulkanTexture->getVkImage();

    auto vulkanBuffer = downcast(buffer.buffer);
    auto dstBuffer = vulkanBuffer->getVkBuffer();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = ToVkImageAspectFlags(texture.aspect);
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { .width = extent.width,
                           .height = extent.height,
                           .depth = extent.depth };

    auto currentLayout = vulkanTexture->getCurrentLayout(texture.mipLevel);
    // change layout
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = currentLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkanTexture->getVkImage();
        barrier.subresourceRange = range;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        vulkanTexture->cmdPipelineBarrier(m_commandBuffer->getVkCommandBuffer(), srcStage, dstStage, barrier);
    }

    // copy texture to buffer
    vkAPI.CmdCopyImageToBuffer(m_commandBuffer->getVkCommandBuffer(), srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer, 1, &region);

    // restore layout
    // TODO: restore layout if need to use the texture in the next command.
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = currentLayout == VK_IMAGE_LAYOUT_UNDEFINED ? vulkanTexture->getFinalLayout() : currentLayout; // TODO: image layout
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vulkanTexture->getVkImage();
        barrier.subresourceRange = range;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // VK_PIPELINE_STAGE_NONE if synchronization2 is enabled.

        vulkanTexture->cmdPipelineBarrier(m_commandBuffer->getVkCommandBuffer(), srcStage, dstStage, barrier);
    }
}

void VulkanCommandRecorder::copyTextureToTexture(CopyTextureToTextureCommand* command)
{
    m_commandResourceSyncronizer.copyTextureToTexture(command);

    auto& src = command->src;
    auto& dst = command->dst;
    auto& extent = command->extent;

    const VulkanAPI& vkAPI = m_commandBuffer->getDevice()->vkAPI;

    // set pipeline barrier to change image layout for src

    auto srcVulkanTexture = downcast(src.texture);
    auto srcCurrentLayout = srcVulkanTexture->getCurrentLayout(src.mipLevel);
    // change layout for src
    {
        VkImageSubresourceRange srcSubresourceRange{};
        srcSubresourceRange.aspectMask = ToVkImageAspectFlags(src.aspect);
        srcSubresourceRange.baseArrayLayer = 0;
        srcSubresourceRange.layerCount = 1;
        srcSubresourceRange.baseMipLevel = src.mipLevel;
        srcSubresourceRange.levelCount = 1;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = srcCurrentLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = srcVulkanTexture->getVkImage();
        barrier.subresourceRange = srcSubresourceRange;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        srcVulkanTexture->cmdPipelineBarrier(m_commandBuffer->getVkCommandBuffer(), srcStage, dstStage, barrier);
    }

    // set pipeline barrier to change image layout for dst

    auto dstVulkanTexture = downcast(dst.texture);
    auto dstCurrentLayout = dstVulkanTexture->getCurrentLayout(dst.mipLevel);
    // change layout for dst
    {
        VkImageSubresourceRange dstSubresourceRange{};
        dstSubresourceRange.aspectMask = ToVkImageAspectFlags(dst.aspect);
        dstSubresourceRange.baseArrayLayer = 0;
        dstSubresourceRange.layerCount = 1;
        dstSubresourceRange.baseMipLevel = dst.mipLevel;
        dstSubresourceRange.levelCount = 1;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = dstCurrentLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = dstVulkanTexture->getVkImage();
        barrier.subresourceRange = dstSubresourceRange;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        dstVulkanTexture->cmdPipelineBarrier(m_commandBuffer->getVkCommandBuffer(), srcStage, dstStage, barrier);
    }

    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = ToVkImageAspectFlags(src.aspect);
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;

    copyRegion.srcOffset = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask = ToVkImageAspectFlags(dst.aspect);
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;

    copyRegion.dstOffset = { 0, 0, 0 };

    copyRegion.extent.width = extent.width;
    copyRegion.extent.height = extent.height;
    copyRegion.extent.depth = extent.depth;

    auto srcImage = srcVulkanTexture->getVkImage();
    auto dstImage = dstVulkanTexture->getVkImage();
    vkAPI.CmdCopyImage(m_commandBuffer->getVkCommandBuffer(),
                       srcImage,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dstImage,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &copyRegion);

    // restore layout for src
    {
        VkImageSubresourceRange srcSubresourceRange{};
        srcSubresourceRange.aspectMask = ToVkImageAspectFlags(src.aspect);
        srcSubresourceRange.baseArrayLayer = 0;
        srcSubresourceRange.layerCount = 1;
        srcSubresourceRange.baseMipLevel = src.mipLevel;
        srcSubresourceRange.levelCount = 1;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = srcCurrentLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = srcVulkanTexture->getVkImage();
        barrier.subresourceRange = srcSubresourceRange;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // VK_PIPELINE_STAGE_NONE if synchronization2 is enabled.

        srcVulkanTexture->cmdPipelineBarrier(m_commandBuffer->getVkCommandBuffer(), srcStage, dstStage, barrier);
    }
    // restore layout for dst
    {
        VkImageSubresourceRange dstSubresourceRange{};
        dstSubresourceRange.aspectMask = ToVkImageAspectFlags(dst.aspect);
        dstSubresourceRange.baseArrayLayer = 0;
        dstSubresourceRange.layerCount = 1;
        dstSubresourceRange.baseMipLevel = dst.mipLevel;
        dstSubresourceRange.levelCount = 1;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = VK_NULL_HANDLE;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = dstCurrentLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = dstVulkanTexture->getVkImage();
        barrier.subresourceRange = dstSubresourceRange;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // VK_PIPELINE_STAGE_NONE if synchronization2 is enabled.

        dstVulkanTexture->cmdPipelineBarrier(m_commandBuffer->getVkCommandBuffer(), srcStage, dstStage, barrier);
    }
}

void VulkanCommandRecorder::resolveQuerySet(ResolveQuerySetCommand* command)
{
    m_commandResourceSyncronizer.resolveQuerySet(command);

    auto querySet = command->querySet;
    auto firstQuery = command->firstQuery;
    auto destination = command->destination;
    auto offset = command->destinationOffset;

    auto vulkanDevice = m_commandBuffer->getDevice();
    auto vulkanQuerySet = downcast(querySet);
    auto vulkanBuffer = downcast(destination);

    std::vector<uint64_t> timestamps(vulkanQuerySet->getCount());

    auto& vkAPI = m_commandBuffer->getDevice()->vkAPI;
    vkAPI.CmdCopyQueryPoolResults(m_commandBuffer->getVkCommandBuffer(),
                                  vulkanQuerySet->getVkQueryPool(),
                                  firstQuery,
                                  vulkanQuerySet->getCount(),
                                  vulkanBuffer->getVkBuffer(),
                                  offset,
                                  sizeof(uint64_t),
                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
}

VulkanCommandBuffer* VulkanCommandRecorder::getCommandBuffer() const
{
    return m_commandBuffer;
}

// Generator
VkPipelineStageFlags generatePipelineStageFlags(Command* cmd)
{
    return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
}

VkAccessFlags generateBufferAccessFlags(BufferUsageFlags usage)
{
    VkAccessFlags flags = 0;

    if (usage & BufferUsageFlagBits::kMapRead)
    {
        flags |= VK_ACCESS_HOST_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kMapWrite)
    {
        flags |= VK_ACCESS_HOST_WRITE_BIT;
    }
    if (usage & BufferUsageFlagBits::kCopySrc)
    {
        flags |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kCopyDst)
    {
        flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (usage & BufferUsageFlagBits::kIndex)
    {
        flags |= VK_ACCESS_INDEX_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kVertex)
    {
        flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kUniform)
    {
        flags |= VK_ACCESS_UNIFORM_READ_BIT;
    }
    if (usage & BufferUsageFlagBits::kStorage)
    {
        flags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (usage & BufferUsageFlagBits::kQueryResolve)
    {
        flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    return flags;
}

VkAccessFlags generateTextureAccessFlags(TextureUsageFlags usage)
{
    VkAccessFlags flags = 0;

    if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        flags |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        flags |= VK_ACCESS_SHADER_READ_BIT;
    }
    if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        flags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    return flags;
}

} // namespace jipu