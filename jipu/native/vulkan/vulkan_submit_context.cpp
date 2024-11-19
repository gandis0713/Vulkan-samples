#include "vulkan_submit_context.h"

#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_sampler.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

void VulkanSubmit::add(VkCommandBuffer commandBuffer)
{
    info.commandBuffers.push_back(commandBuffer);
}

void VulkanSubmit::addSignalSemaphore(const std::vector<VkSemaphore>& semaphores)
{
    if (semaphores.empty())
    {
        return;
    }

    info.signalSemaphores.insert(info.signalSemaphores.end(), semaphores.begin(), semaphores.end());
}

void VulkanSubmit::addWaitSemaphore(const std::vector<VkSemaphore>& semaphores, const std::vector<VkPipelineStageFlags>& stage)
{
    if (semaphores.size() != stage.size())
    {
        throw std::runtime_error("Failed to add wait semaphore. The size of semaphores and stages are not matched.");
    }

    if (semaphores.empty() || stage.empty())
    {
        return;
    }

    info.waitSemaphores.insert(info.waitSemaphores.end(), semaphores.begin(), semaphores.end());
    info.waitStages.insert(info.waitStages.end(), stage.begin(), stage.end());
}

void VulkanSubmit::add(VkImageView imageView)
{
    object.imageViews.insert(imageView);
}

void VulkanSubmit::add(VkSampler sampler)
{
    object.samplers.insert(sampler);
}

void VulkanSubmit::add(VkPipeline pipeline)
{
    object.pipelines.insert(pipeline);
}

void VulkanSubmit::add(VkPipelineLayout pipelineLayout)
{
    object.pipelineLayouts.insert(pipelineLayout);
}

void VulkanSubmit::add(VkDescriptorSet descriptorSet)
{
    object.descriptorSet.insert(descriptorSet);
}

void VulkanSubmit::add(VkDescriptorSetLayout descriptorSetLayout)
{
    object.descriptorSetLayouts.insert(descriptorSetLayout);
}

void VulkanSubmit::add(VkFramebuffer framebuffer)
{
    object.framebuffers.insert(framebuffer);
}

void VulkanSubmit::add(VkRenderPass renderPass)
{
    object.renderPasses.insert(renderPass);
}

void VulkanSubmit::addSrcBuffer(VulkanBufferResource buffer)
{
    object.srcResource.buffers.insert({ buffer.buffer, buffer.memory });
}

void VulkanSubmit::addSrcImage(VulkanTextureResource image)
{
    object.srcResource.images.insert({ image.image, image.memory });
}

void VulkanSubmit::addDstBuffer(VulkanBufferResource buffer)
{
    object.srcResource.buffers.insert({ buffer.buffer, buffer.memory });
}

void VulkanSubmit::addDstImage(VulkanTextureResource image)
{
    object.srcResource.images.insert({ image.image, image.memory });
}

VulkanSubmitContext VulkanSubmitContext::create(VulkanDevice* device, const std::vector<VulkanCommandRecordResult>& results)
{
    VulkanSubmitContext context{};
    {
        auto findSrcBuffer = [](const std::vector<VulkanCommandRecordResult>& submittedResults, Buffer* buffer) -> bool {
            auto it = std::find_if(submittedResults.begin(), submittedResults.end(), [buffer](const VulkanCommandRecordResult& result) {
                const auto& notSyncedInfos = result.commandResourceSyncResult.notSyncedPassResourceInfos;
                return std::find_if(notSyncedInfos.begin(), notSyncedInfos.end(), [buffer](const PassResourceInfo& info) {
                           return info.src.buffers.contains(buffer);
                       }) != notSyncedInfos.end();
            });

            return it != submittedResults.end();
        };

        auto findSrcTexture = [&](const std::vector<VulkanCommandRecordResult>& submittedResults, Texture* texture) -> bool {
            auto it = std::find_if(submittedResults.begin(), submittedResults.end(), [texture](const VulkanCommandRecordResult& result) {
                const auto& notSyncedInfos = result.commandResourceSyncResult.notSyncedPassResourceInfos;
                return std::find_if(notSyncedInfos.begin(), notSyncedInfos.end(), [texture](const PassResourceInfo& info) {
                           return info.src.textures.contains(texture);
                       }) != notSyncedInfos.end();
            });

            return it != submittedResults.end();
        };

        auto findSrcResource = [&](const std::vector<VulkanCommandRecordResult>& submittedResults, const std::vector<PassResourceInfo>& notSyncedInfos) -> bool {
            for (const auto& info : notSyncedInfos)
            {
                for (const auto& [buffer, _] : info.dst.buffers)
                {
                    if (findSrcBuffer(submittedResults, buffer))
                    {
                        return true;
                    }
                }

                for (const auto& [texture, _] : info.src.textures)
                {
                    if (findSrcTexture(submittedResults, texture))
                    {
                        return true;
                    }
                }
            }

            return false;
        };

        auto getSrcBufferUsageInfo = [&](const std::vector<VulkanCommandRecordResult>& submittedResults, Buffer* buffer) -> BufferUsageInfo {
            for (auto resultIter = submittedResults.rbegin(); resultIter != submittedResults.rend(); ++resultIter)
            {
                const auto& result = *resultIter;
                const auto& notSyncedInfos = result.commandResourceSyncResult.notSyncedPassResourceInfos;
                for (auto infoIter = notSyncedInfos.rbegin(); infoIter != notSyncedInfos.rend(); ++infoIter)
                {
                    const auto& info = *infoIter;
                    if (info.src.buffers.contains(buffer))
                    {
                        return info.src.buffers.at(buffer);
                    }
                }
            }

            spdlog::error("Failed to find source buffer usage info.");
            return {};
        };

        auto getSrcTextureUsageInfo = [&](const std::vector<VulkanCommandRecordResult>& submittedResults, Texture* texture) -> TextureUsageInfo {
            for (auto resultIter = submittedResults.rbegin(); resultIter != submittedResults.rend(); ++resultIter)
            {
                const auto& result = *resultIter;
                const auto& notSyncedInfos = result.commandResourceSyncResult.notSyncedPassResourceInfos;
                for (auto infoIter = notSyncedInfos.rbegin(); infoIter != notSyncedInfos.rend(); ++infoIter)
                {
                    const auto& info = *infoIter;
                    if (info.src.textures.contains(texture))
                    {
                        return info.src.textures.at(texture);
                    }
                }
            }

            spdlog::error("Failed to find source texture usage info.");
            return {};
        };

        auto getSrcResourceUsageInfo = [&](const std::vector<VulkanCommandRecordResult>& submittedResults, const std::vector<PassResourceInfo>& notSyncedInfos) -> ResourceInfo {
            ResourceInfo srcResourceInfo{};

            for (const auto& info : notSyncedInfos)
            {
                for (const auto& [buffer, _] : info.dst.buffers)
                {
                    if (findSrcBuffer(submittedResults, buffer))
                    {
                        srcResourceInfo.buffers[buffer] = getSrcBufferUsageInfo(results, buffer);
                    }
                }

                for (const auto& [texture, _] : info.src.textures)
                {
                    if (findSrcTexture(submittedResults, texture))
                    {
                        srcResourceInfo.textures[texture] = getSrcTextureUsageInfo(results, texture);
                    }
                }
            }

            return srcResourceInfo;
        };

        auto getSrcBufferSemaphores = [](std::vector<VulkanSubmit>& submits, Buffer* buffer) -> std::vector<VkSemaphore> {
            std::vector<VkSemaphore> semaphores{};
            for (const auto& submit : submits)
            {
                if (submit.object.srcResource.buffers.contains(downcast(buffer)->getVkBuffer()))
                {
                    semaphores.insert(semaphores.end(), submit.info.signalSemaphores.begin(), submit.info.signalSemaphores.end());
                }
            }

            return semaphores;
        };

        auto getSrcTextureSemaphores = [](std::vector<VulkanSubmit>& submits, Texture* texture) -> std::vector<VkSemaphore> {
            std::vector<VkSemaphore> semaphores{};
            for (const auto& submit : submits)
            {
                if (submit.object.srcResource.images.contains(downcast(texture)->getVkImage()))
                {
                    semaphores.insert(semaphores.end(), submit.info.signalSemaphores.begin(), submit.info.signalSemaphores.end());
                }
            }

            return semaphores;
        };

        auto getSubmitType = [](const VulkanCommandRecordResult& result) -> SubmitType {
            const auto& notSyncedInfos = result.commandResourceSyncResult.notSyncedPassResourceInfos;
            if (notSyncedInfos.empty())
            {
                // assumed copied resources.
                return SubmitType::kTransfer;
            }
            const auto& info = notSyncedInfos[notSyncedInfos.size() - 1]; // only last pass resource info

            const auto& src = info.src;

            if (src.buffers.empty() && src.textures.empty())
            {
                spdlog::error("There is no output resource.");
                return SubmitType::kNone;
            }

            for (const auto& [texture, textureUsageInfo] : src.textures)
            {
                if (textureUsageInfo.stageFlags & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
                {
                    auto owner = downcast(texture)->getOwner();

                    switch (owner)
                    {
                    case VulkanTextureOwner::kSwapchain:
                        return SubmitType::kPresent;
                    case VulkanTextureOwner::kSelf:
                    default:
                        return SubmitType::kRender;
                    }
                }
                if (textureUsageInfo.stageFlags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
                {
                    return SubmitType::kCompute;
                }

                if (textureUsageInfo.stageFlags & VK_PIPELINE_STAGE_TRANSFER_BIT)
                {
                    return SubmitType::kTransfer;
                }
            }

            for (const auto& [_, bufferUsageInfo] : src.buffers)
            {
                if (bufferUsageInfo.stageFlags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
                {
                    return SubmitType::kCompute;
                }
                if (bufferUsageInfo.stageFlags & VK_PIPELINE_STAGE_TRANSFER_BIT)
                {
                    return SubmitType::kTransfer;
                }
            }

            return SubmitType::kNone;
        };

        VulkanSubmit currentSubmit{};
        std::vector<VulkanCommandRecordResult> submittedRecordResults{};
        for (const auto& result : results)
        {
            // generate submit info
            {
                auto hasSrcDependency = findSrcResource(submittedRecordResults, result.commandResourceSyncResult.notSyncedPassResourceInfos);

                if (hasSrcDependency)
                {
                    // prepare next submit info.
                    // because we need to wait for the previous submit by semaphore.
                    context.m_submits.push_back(currentSubmit);
                    currentSubmit = {};

                    // set signal semaphore
                    {
                        auto semaphore = device->getSemaphorePool()->create();
                        currentSubmit.addSignalSemaphore({ semaphore });
                    }
                }

                // set command buffer
                {
                    auto commandBuffer = downcast(result.commandBuffer);
                    currentSubmit.add(commandBuffer->getVkCommandBuffer());
                }

                // set wait semaphore
                {
                    if (hasSrcDependency) // set wait semaphore and resource info
                    {
                        std::vector<VkPipelineStageFlags> waitStages{};
                        std::vector<VkSemaphore> waitSemaphores{};

                        auto& notSynedPassResourceInfos = result.commandResourceSyncResult.notSyncedPassResourceInfos;

                        for (const auto& notSynedPassResourceInfo : notSynedPassResourceInfos)
                        {
                            for (const auto& [dstBuffer, dstBufferUsageInfo] : notSynedPassResourceInfo.dst.buffers)
                            {
                                if (findSrcBuffer(submittedRecordResults, dstBuffer))
                                {
                                    auto bufferWaitSemaphores = getSrcBufferSemaphores(context.m_submits, dstBuffer);
                                    waitSemaphores.insert(waitSemaphores.end(), bufferWaitSemaphores.begin(), bufferWaitSemaphores.end());

                                    waitStages.push_back(dstBufferUsageInfo.stageFlags);
                                }
                            }

                            for (const auto& [dstTexture, dstTextureUsageInfo] : notSynedPassResourceInfo.dst.textures)
                            {
                                if (findSrcTexture(submittedRecordResults, dstTexture))
                                {
                                    auto bufferWaitSemaphores = getSrcTextureSemaphores(context.m_submits, dstTexture);
                                    waitSemaphores.insert(waitSemaphores.end(), bufferWaitSemaphores.begin(), bufferWaitSemaphores.end());

                                    waitStages.push_back(dstTextureUsageInfo.stageFlags);
                                }
                            }
                        }

                        currentSubmit.addWaitSemaphore(waitSemaphores, waitStages);
                    }

                    // set wait semaphore and image index for swapchain image
                    {
                        if (getSubmitType(result) == SubmitType::kPresent)
                        {
                            auto& notSynedPassResourceInfos = result.commandResourceSyncResult.notSyncedPassResourceInfos;
                            for (const auto& notSynedPassResourceInfo : notSynedPassResourceInfos)
                            {
                                for (const auto& [dstTexture, dstTextureUsageInfo] : notSynedPassResourceInfo.dst.textures)
                                {
                                    VulkanTexture* vulkanTexture = downcast(dstTexture);
                                    if (vulkanTexture->getOwner() == VulkanTextureOwner::kSwapchain)
                                    {
                                        VulkanSwapchainTexture* vulkanSwapchainTexture = downcast(vulkanTexture);

                                        VkSemaphore semaphore = vulkanSwapchainTexture->getAcquireSemaphore();
                                        VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                        currentSubmit.addWaitSemaphore({ semaphore }, { flags });

                                        currentSubmit.info.swapchainIndex = vulkanSwapchainTexture->getImageIndex();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // set signal semaphore for external resource
                {
                    // TODO
                }

                // set submit type
                {
                    currentSubmit.info.type = getSubmitType(result);
                }
            }

            // generate submit objects
            {
                auto vulkanCommandBuffer = downcast(result.commandBuffer);
                auto& commands = vulkanCommandBuffer->getCommandEncodingResult().commands;
                for (auto& command : commands)
                {
                    switch (command->type)
                    {
                    case CommandType::kBeginComputePass:
                    case CommandType::kEndComputePass:
                    case CommandType::kDispatch:
                    case CommandType::kDispatchIndirect:
                    default:
                        break;
                    case CommandType::kSetComputePipeline: {
                        auto cmd = reinterpret_cast<SetComputePipelineCommand*>(command.get());
                        currentSubmit.add(downcast(cmd->pipeline)->getVkPipeline());
                        currentSubmit.add(downcast(cmd->pipeline->getPipelineLayout())->getVkPipelineLayout());
                    }
                    break;
                    case CommandType::kSetComputeBindGroup: {
                        auto cmd = reinterpret_cast<SetBindGroupCommand*>(command.get());
                        currentSubmit.add(downcast(cmd->bindGroup)->getVkDescriptorSet());
                        currentSubmit.add(downcast(cmd->bindGroup->getLayout())->getVkDescriptorSetLayout());
                        for (auto& binding : cmd->bindGroup->getBufferBindings())
                        {
                            currentSubmit.addSrcBuffer(downcast(binding.buffer)->getVulkanBufferResource());
                            currentSubmit.addDstBuffer(downcast(binding.buffer)->getVulkanBufferResource());
                        }
                        for (auto& binding : cmd->bindGroup->getSmaplerBindings())
                        {
                            currentSubmit.add(downcast(binding.sampler)->getVkSampler());
                        }
                        for (auto& binding : cmd->bindGroup->getTextureBindings())
                        {
                            currentSubmit.addSrcImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());
                            currentSubmit.addDstImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());

                            currentSubmit.add(downcast(binding.textureView)->getVkImageView());
                        }
                    }
                    break;
                    case CommandType::kBeginRenderPass: {
                        auto cmd = reinterpret_cast<BeginRenderPassCommand*>(command.get());
                        for (auto& colorAttachment : cmd->framebuffer->getColorAttachments())
                        {
                            currentSubmit.addSrcImage(downcast(colorAttachment.renderView->getTexture())->getVulkanTextureResource());
                            currentSubmit.add(downcast(colorAttachment.renderView)->getVkImageView());
                            if (colorAttachment.resolveView)
                            {
                                currentSubmit.addSrcImage(downcast(colorAttachment.resolveView->getTexture())->getVulkanTextureResource());
                                currentSubmit.add(downcast(colorAttachment.resolveView)->getVkImageView());
                            }
                        }
                        currentSubmit.add(cmd->framebuffer->getVkFrameBuffer());
                        currentSubmit.add(cmd->renderPass->getVkRenderPass());
                    }
                    break;
                    case CommandType::kSetRenderPipeline: {
                        auto cmd = reinterpret_cast<SetRenderPipelineCommand*>(command.get());
                        currentSubmit.add(downcast(cmd->pipeline)->getVkPipeline());
                        currentSubmit.add(downcast(cmd->pipeline->getPipelineLayout())->getVkPipelineLayout());
                    }
                    break;
                    case CommandType::kSetRenderBindGroup: {
                        auto cmd = reinterpret_cast<SetBindGroupCommand*>(command.get());
                        currentSubmit.add(downcast(cmd->bindGroup)->getVkDescriptorSet());
                        currentSubmit.add(downcast(cmd->bindGroup->getLayout())->getVkDescriptorSetLayout());
                        for (auto& binding : cmd->bindGroup->getBufferBindings())
                        {
                            currentSubmit.addSrcBuffer(downcast(binding.buffer)->getVulkanBufferResource());
                            currentSubmit.addDstBuffer(downcast(binding.buffer)->getVulkanBufferResource());
                        }
                        for (auto& binding : cmd->bindGroup->getSmaplerBindings())
                        {
                            currentSubmit.add(downcast(binding.sampler)->getVkSampler());
                        }
                        for (auto& binding : cmd->bindGroup->getTextureBindings())
                        {
                            currentSubmit.addSrcImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());
                            currentSubmit.addDstImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());

                            currentSubmit.add(downcast(binding.textureView)->getVkImageView());
                        }
                    }
                    break;
                    case CommandType::kSetIndexBuffer: {
                        auto cmd = reinterpret_cast<SetIndexBufferCommand*>(command.get());
                        currentSubmit.addDstBuffer(downcast(cmd->buffer)->getVulkanBufferResource());
                    }
                    break;
                    case CommandType::kSetVertexBuffer: {
                        auto cmd = reinterpret_cast<SetVertexBufferCommand*>(command.get());
                        currentSubmit.addDstBuffer(downcast(cmd->buffer)->getVulkanBufferResource());
                    }
                    break;
                    }
                }
            }

            submittedRecordResults.push_back(result);
        }

        context.m_submits.push_back(currentSubmit);
    }

    return context;
}

const std::vector<VulkanSubmit>& VulkanSubmitContext::getSubmits() const
{
    return m_submits;
}

std::vector<VulkanSubmit::Info> VulkanSubmitContext::getSubmitInfos() const
{
    std::vector<VulkanSubmit::Info> submitInfos{};
    for (const auto& submit : m_submits)
    {
        submitInfos.push_back(submit.info);
    }

    return submitInfos;
}

std::vector<VulkanSubmit::Object> VulkanSubmitContext::getSubmitObjects() const
{
    std::vector<VulkanSubmit::Object> submitObjects{};
    for (const auto& submit : m_submits)
    {
        submitObjects.push_back(submit.object);
    }

    return submitObjects;
}

} // namespace jipu