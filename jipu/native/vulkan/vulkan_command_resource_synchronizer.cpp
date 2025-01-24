#include "vulkan_command_resource_synchronizer.h"

#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_command.h"
#include "vulkan_command_encoder.h"
#include "vulkan_command_recorder.h"
#include "vulkan_device.h"
#include "vulkan_texture.h"

#include <spdlog/spdlog.h>

namespace jipu
{
VulkanCommandResourceSynchronizer::VulkanCommandResourceSynchronizer(VulkanCommandRecorder* commandRecorder, const VulkanCommandResourceSynchronizerDescriptor& descriptor)
    : m_commandRecorder(commandRecorder)
    , m_operationResourceInfos(descriptor.operationResourceInfos)
    , m_currentOperationIndex(-1)
{
}

void VulkanCommandResourceSynchronizer::beginComputePass(BeginComputePassCommand* command)
{
    increaseOperationIndex();
}

void VulkanCommandResourceSynchronizer::setComputePipeline(SetComputePipelineCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::setComputeBindGroup(SetBindGroupCommand* command)
{
    auto bufferBindings = command->bindGroup->getBufferBindings();
    for (auto& bufferBinding : bufferBindings)
    {
        m_activatedDstResource.buffers.insert(bufferBinding.buffer);
    }

    auto textureBindings = command->bindGroup->getTextureBindings();
    for (auto& textureBinding : textureBindings)
    {
        m_activatedDstResource.textureViews.insert(textureBinding.textureView);
    }
}

void VulkanCommandResourceSynchronizer::dispatch(DispatchCommand* command)
{
    sync();
}

void VulkanCommandResourceSynchronizer::dispatchIndirect(DispatchIndirectCommand* command)
{
    // TODO
}

void VulkanCommandResourceSynchronizer::endComputePass(EndComputePassCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::beginRenderPass(BeginRenderPassCommand* command)
{
    increaseOperationIndex();

    // all dst buffer resources in a render pass are active
    for (auto& [buffer, _] : m_operationResourceInfos[currentOperationIndex()].dst.buffers)
    {
        m_activatedDstResource.buffers.insert(buffer);
    }

    // all dst texture resources in a render pass are active
    for (auto& [texture, _] : m_operationResourceInfos[currentOperationIndex()].dst.textureViews)
    {
        m_activatedDstResource.textureViews.insert(texture);
    }

    // all resources in a render pass should be synchronized before the render pass
    sync();
}

void VulkanCommandResourceSynchronizer::setRenderPipeline(SetRenderPipelineCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::setVertexBuffer(SetVertexBufferCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::setIndexBuffer(SetIndexBufferCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::setViewport(SetViewportCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::setScissor(SetScissorCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::setBlendConstant(SetBlendConstantCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::draw(DrawCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::drawIndexed(DrawIndexedCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::beginOcclusionQuery(BeginOcclusionQueryCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::endOcclusionQuery(EndOcclusionQueryCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::endRenderPass(EndRenderPassCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::setRenderBindGroup(SetBindGroupCommand* command)
{
    auto bufferBindings = command->bindGroup->getBufferBindings();
    for (auto& bufferBinding : bufferBindings)
    {
        m_activatedDstResource.buffers.insert(bufferBinding.buffer);
    }

    auto textureBindings = command->bindGroup->getTextureBindings();
    for (auto& textureBinding : textureBindings)
    {
        m_activatedDstResource.textureViews.insert(textureBinding.textureView);
    }
}

void VulkanCommandResourceSynchronizer::copyBufferToBuffer(CopyBufferToBufferCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::copyBufferToTexture(CopyBufferToTextureCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::copyTextureToBuffer(CopyTextureToBufferCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::copyTextureToTexture(CopyTextureToTextureCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceSynchronizer::resolveQuerySet(ResolveQuerySetCommand* command)
{
    // do nothing.
}

ResourceSyncResult VulkanCommandResourceSynchronizer::finish()
{
    return ResourceSyncResult{ .notSyncedOperationResourceInfos = m_operationResourceInfos };
}

void VulkanCommandResourceSynchronizer::cmdPipelineBarrier(const PipelineBarrier& barrier)
{
    auto& srcStageMask = barrier.srcStageMask;
    auto& dstStageMask = barrier.dstStageMask;
    auto& dependencyFlags = barrier.dependencyFlags;
    auto& memoryBarriers = barrier.memoryBarriers;
    auto& bufferMemoryBarriers = barrier.bufferMemoryBarriers;
    auto& imageMemoryBarriers = barrier.imageMemoryBarriers;

    auto vulkanDevice = m_commandRecorder->getCommandBuffer()->getDevice();
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    vkAPI.CmdPipelineBarrier(m_commandRecorder->getCommandBuffer()->getVkCommandBuffer(),
                             srcStageMask,
                             dstStageMask,
                             dependencyFlags,
                             static_cast<uint32_t>(memoryBarriers.size()),
                             memoryBarriers.data(),
                             static_cast<uint32_t>(memoryBarriers.size()),
                             bufferMemoryBarriers.data(),
                             static_cast<uint32_t>(memoryBarriers.size()),
                             imageMemoryBarriers.data());
}

bool VulkanCommandResourceSynchronizer::findSrcBuffer(Buffer* buffer) const
{
    auto& operationResourceInfos = m_operationResourceInfos;

    auto begin = operationResourceInfos.begin();
    auto end = operationResourceInfos.begin() + currentOperationIndex();
    auto it = std::find_if(begin, end, [buffer](const OperationResourceInfo& operationResourceInfo) {
        return operationResourceInfo.src.buffers.contains(buffer);
    });

    return it != end;
}

bool VulkanCommandResourceSynchronizer::findSrcTextureView(TextureView* textureView) const
{
    auto& operationResourceInfos = m_operationResourceInfos;

    auto begin = operationResourceInfos.begin();
    auto end = operationResourceInfos.begin() + currentOperationIndex();
    auto it = std::find_if(begin, end, [textureView](const OperationResourceInfo& operationResourceInfo) {
        return operationResourceInfo.src.textureViews.contains(textureView);
    });

    return it != end;
}

BufferUsageInfo VulkanCommandResourceSynchronizer::extractSrcBufferUsageInfo(Buffer* buffer)
{
    auto& operationResourceInfos = m_operationResourceInfos;

    auto begin = operationResourceInfos.begin();
    auto end = operationResourceInfos.begin() + currentOperationIndex();
    auto it = std::find_if(begin, end, [buffer](const OperationResourceInfo& operationResourceInfo) {
        return operationResourceInfo.src.buffers.find(buffer) != operationResourceInfo.src.buffers.end();
    });

    auto bufferUsageInfo = it->src.buffers.at(buffer);
    it->src.buffers.erase(buffer); // remove it

    return bufferUsageInfo;
}

TextureUsageInfo VulkanCommandResourceSynchronizer::extractSrcTextureUsageInfo(TextureView* textureView)
{
    auto& operationResourceInfos = m_operationResourceInfos;

    auto begin = operationResourceInfos.begin();
    auto end = operationResourceInfos.begin() + currentOperationIndex();
    auto it = std::find_if(begin, end, [textureView](const OperationResourceInfo& operationResourceInfo) {
        return operationResourceInfo.src.textureViews.find(textureView) != operationResourceInfo.src.textureViews.end();
    });

    auto textureUsageInfo = it->src.textureViews.at(textureView);
    it->src.textureViews.erase(textureView); // remove it

    return textureUsageInfo;
}

OperationResourceInfo& VulkanCommandResourceSynchronizer::getCurrentOperationResourceInfo()
{
    return m_operationResourceInfos[currentOperationIndex()];
}

void VulkanCommandResourceSynchronizer::increaseOperationIndex()
{
    ++m_currentOperationIndex;
}

int32_t VulkanCommandResourceSynchronizer::currentOperationIndex() const
{
    return m_currentOperationIndex;
}

void VulkanCommandResourceSynchronizer::sync()
{
    VkCommandBuffer commandBuffer = m_commandRecorder->getCommandBuffer()->getVkCommandBuffer();
    auto& currentOperationResourceInfo = getCurrentOperationResourceInfo();

    // spdlog::trace("current operation buffers src: {}", currentOperationResourceInfo.src.buffers.size());
    // spdlog::trace("current operation textureViews src: {}", currentOperationResourceInfo.src.textureViews.size());

    // spdlog::trace("current operation buffers dst: {}", currentOperationResourceInfo.dst.buffers.size());
    // spdlog::trace("current operation textureViews dst: {}", currentOperationResourceInfo.dst.textureViews.size());

    // buffers
    auto& currentDstOperationBuffers = currentOperationResourceInfo.dst.buffers;
    for (auto it = currentDstOperationBuffers.begin(); it != currentDstOperationBuffers.end();)
    {
        auto buffer = it->first;
        auto dstBufferUsageInfo = it->second;

        if (m_activatedDstResource.buffers.contains(buffer))
        {
            if (findSrcBuffer(buffer))
            {
                auto vulkanBuffer = downcast(buffer);
                auto srcBufferUsageInfo = extractSrcBufferUsageInfo(buffer); // extract src resource

                VkBufferMemoryBarrier bufferMemoryBarrier{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = srcBufferUsageInfo.accessFlags,
                    .dstAccessMask = dstBufferUsageInfo.accessFlags,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = vulkanBuffer->getVkBuffer(),
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
                };

                it = currentDstOperationBuffers.erase(it); // extract dst resource

                vulkanBuffer->cmdPipelineBarrier(commandBuffer, srcBufferUsageInfo.stageFlags, dstBufferUsageInfo.stageFlags, bufferMemoryBarrier);
                continue;
            }
        }

        ++it; // increase iterator
    }

    // textures
    auto& currentDstOperationTextureViews = currentOperationResourceInfo.dst.textureViews;
    for (auto it = currentDstOperationTextureViews.begin(); it != currentDstOperationTextureViews.end();)
    {
        auto textureView = it->first;
        auto dstTextureUsageInfo = it->second;

        if (m_activatedDstResource.textureViews.contains(textureView))
        {
            if (findSrcTextureView(textureView))
            {
                auto srcTextureUsageInfo = extractSrcTextureUsageInfo(textureView);

                auto vulkanTexture = downcast(textureView->getTexture());
                VkImageMemoryBarrier imageMemoryBarrier{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = srcTextureUsageInfo.accessFlags,
                    .dstAccessMask = dstTextureUsageInfo.accessFlags,
                    .oldLayout = srcTextureUsageInfo.layout,
                    .newLayout = dstTextureUsageInfo.layout,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = vulkanTexture->getVkImage(),
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = dstTextureUsageInfo.baseMipLevel,
                        .levelCount = dstTextureUsageInfo.mipLevelCount,
                        .baseArrayLayer = dstTextureUsageInfo.baseArrayLayer,
                        .layerCount = dstTextureUsageInfo.arrayLayerCount,
                    },
                };

                it = currentDstOperationTextureViews.erase(it); // extract dst resource

                vulkanTexture->cmdPipelineBarrier(commandBuffer, srcTextureUsageInfo.stageFlags, dstTextureUsageInfo.stageFlags, imageMemoryBarrier);
                continue;
            }
        }

        ++it; // increase iterator
    }

    // clear active resource
    m_activatedDstResource.clear();
}

} // namespace jipu