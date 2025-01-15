#include "vulkan_submit_context.h"

#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_render_bundle.h"
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

void VulkanSubmit::add(CopyBufferToBufferCommand* command)
{
    addSrcBuffer(downcast(command->src.buffer)->getVulkanBufferResource());
    addDstBuffer(downcast(command->dst.buffer)->getVulkanBufferResource());
}

void VulkanSubmit::add(CopyBufferToTextureCommand* command)
{
    addSrcBuffer(downcast(command->buffer.buffer)->getVulkanBufferResource());
    addDstImage(downcast(command->texture.texture)->getVulkanTextureResource());
}

void VulkanSubmit::add(CopyTextureToBufferCommand* command)
{
    addSrcImage(downcast(command->texture.texture)->getVulkanTextureResource());
    addDstBuffer(downcast(command->buffer.buffer)->getVulkanBufferResource());
}

void VulkanSubmit::add(CopyTextureToTextureCommand* command)
{
    addSrcImage(downcast(command->src.texture)->getVulkanTextureResource());
    addDstImage(downcast(command->dst.texture)->getVulkanTextureResource());
}

void VulkanSubmit::add(SetComputePipelineCommand* command)
{
    add(downcast(command->pipeline)->getVkPipeline());
    add(downcast(command->pipeline)->getVkPipelineLayout());
}

void VulkanSubmit::addComputeBindGroup(SetBindGroupCommand* command)
{
    add(downcast(command->bindGroup)->getVkDescriptorSet());
    for (auto& binding : command->bindGroup->getBufferBindings())
    {
        addSrcBuffer(downcast(binding.buffer)->getVulkanBufferResource());
        addDstBuffer(downcast(binding.buffer)->getVulkanBufferResource());
    }
    for (auto& binding : command->bindGroup->getSmaplerBindings())
    {
        add(downcast(binding.sampler)->getVkSampler());
    }
    for (auto& binding : command->bindGroup->getTextureBindings())
    {
        addSrcImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());
        addDstImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());

        add(downcast(binding.textureView)->getVkImageView());
    }
}

void VulkanSubmit::add(BeginRenderPassCommand* command)
{
    auto framebuffer = command->framebuffer.lock();
    if (framebuffer)
    {
        for (auto& colorAttachment : framebuffer->getColorAttachments())
        {
            addSrcImage(downcast(colorAttachment.renderView->getTexture())->getVulkanTextureResource());
            add(downcast(colorAttachment.renderView)->getVkImageView());
            if (colorAttachment.resolveView)
            {
                addSrcImage(downcast(colorAttachment.resolveView->getTexture())->getVulkanTextureResource());
                add(downcast(colorAttachment.resolveView)->getVkImageView());
            }
        }

        auto depthStencilAttachment = framebuffer->getDepthStencilAttachment();
        if (depthStencilAttachment)
        {
            addSrcImage(downcast(depthStencilAttachment->getTexture())->getVulkanTextureResource());
            add(downcast(depthStencilAttachment)->getVkImageView());
        }
        add(framebuffer->getVkFrameBuffer());
    }

    auto renderPass = command->renderPass.lock();
    if (renderPass)
    {
        add(renderPass->getVkRenderPass());
    }
}

void VulkanSubmit::addRenderBindGroup(SetBindGroupCommand* command)
{
    add(downcast(command->bindGroup)->getVkDescriptorSet());
    for (auto& binding : command->bindGroup->getBufferBindings())
    {
        addSrcBuffer(downcast(binding.buffer)->getVulkanBufferResource());
        addDstBuffer(downcast(binding.buffer)->getVulkanBufferResource());
    }
    for (auto& binding : command->bindGroup->getSmaplerBindings())
    {
        add(downcast(binding.sampler)->getVkSampler());
    }
    for (auto& binding : command->bindGroup->getTextureBindings())
    {
        addSrcImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());
        addDstImage(downcast(binding.textureView->getTexture())->getVulkanTextureResource());

        add(downcast(binding.textureView)->getVkImageView());
    }
}

void VulkanSubmit::add(SetRenderPipelineCommand* command)
{
    add(downcast(command->pipeline)->getVkPipeline());
    add(downcast(command->pipeline)->getVkPipelineLayout());
}

void VulkanSubmit::add(SetVertexBufferCommand* command)
{
    addDstBuffer(downcast(command->buffer)->getVulkanBufferResource());
}

void VulkanSubmit::add(SetIndexBufferCommand* command)
{
    addDstBuffer(downcast(command->buffer)->getVulkanBufferResource());
}

void VulkanSubmit::add(ExecuteBundleCommand* command)
{
    for (auto& renderBundle : command->renderBundles)
    {
        auto vulkanRenderBundle = downcast(renderBundle);
        const auto& commands = vulkanRenderBundle->getCommands();
        for (auto& cmd : commands)
        {
            switch (cmd->type)
            {
            case CommandType::kSetRenderPipeline:
                add(reinterpret_cast<SetRenderPipelineCommand*>(cmd.get()));
                break;
            case CommandType::kSetVertexBuffer:
                add(reinterpret_cast<SetVertexBufferCommand*>(cmd.get()));
                break;
            case CommandType::kSetIndexBuffer:
                add(reinterpret_cast<SetIndexBufferCommand*>(cmd.get()));
                break;
            case CommandType::kSetRenderBindGroup:
                addRenderBindGroup(reinterpret_cast<SetBindGroupCommand*>(cmd.get()));
                break;
            case CommandType::kDraw:
            case CommandType::kDrawIndexed:
            case CommandType::kDrawIndirect:
            case CommandType::kDrawIndexedIndirect:
                // nothing to do.
                break;
            default:
                throw std::runtime_error("Unknown command type.");
                break;
            }
        }
    }
}

bool findSrcBuffer(const std::vector<VulkanCommandBuffer*>& submittedCommandBuffers, Buffer* buffer)
{
    auto it = std::find_if(submittedCommandBuffers.begin(), submittedCommandBuffers.end(), [buffer](VulkanCommandBuffer* commandBuffer) {
        const auto& commandResourceInfos = commandBuffer->getCommandResourceInfos();
        return std::find_if(commandResourceInfos.begin(), commandResourceInfos.end(), [buffer](const OperationResourceInfo& info) {
                   return info.src.buffers.contains(buffer);
               }) != commandResourceInfos.end();
    });

    return it != submittedCommandBuffers.end();
};

bool findSrcTexture(const std::vector<VulkanCommandBuffer*>& submittedCommandBuffers, Texture* texture)
{
    auto it = std::find_if(submittedCommandBuffers.begin(), submittedCommandBuffers.end(), [texture](VulkanCommandBuffer* commandBuffer) {
        const auto& commandResourceInfos = commandBuffer->getCommandResourceInfos();
        return std::find_if(commandResourceInfos.begin(), commandResourceInfos.end(), [texture](const OperationResourceInfo& info) {
                   return info.src.textures.contains(texture);
               }) != commandResourceInfos.end();
    });

    return it != submittedCommandBuffers.end();
};

bool findSrcResource(const std::vector<VulkanCommandBuffer*>& submittedCommandBuffers, const std::vector<OperationResourceInfo>& commandResourceInfos)
{
    for (const auto& info : commandResourceInfos)
    {
        for (const auto& [buffer, _] : info.dst.buffers)
        {
            if (findSrcBuffer(submittedCommandBuffers, buffer))
            {
                return true;
            }
        }

        for (const auto& [texture, _] : info.src.textures)
        {
            if (findSrcTexture(submittedCommandBuffers, texture))
            {
                return true;
            }
        }
    }

    return false;
};

BufferUsageInfo getSrcBufferUsageInfo(const std::vector<VulkanCommandBuffer*>& submittedCommandBuffers, Buffer* buffer)
{
    for (auto resultIter = submittedCommandBuffers.rbegin(); resultIter != submittedCommandBuffers.rend(); ++resultIter)
    {
        const auto& commandResourceInfos = (*resultIter)->getCommandResourceInfos();
        for (auto infoIter = commandResourceInfos.rbegin(); infoIter != commandResourceInfos.rend(); ++infoIter)
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

TextureUsageInfo getSrcTextureUsageInfo(const std::vector<VulkanCommandBuffer*>& submittedCommandBuffers, Texture* texture)
{
    for (auto resultIter = submittedCommandBuffers.rbegin(); resultIter != submittedCommandBuffers.rend(); ++resultIter)
    {
        const auto& commandResourceInfos = (*resultIter)->getCommandResourceInfos();
        for (auto infoIter = commandResourceInfos.rbegin(); infoIter != commandResourceInfos.rend(); ++infoIter)
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

ResourceInfo getSrcResourceUsageInfo(const std::vector<VulkanCommandBuffer*>& submittedCommandBuffers, const std::vector<OperationResourceInfo>& commandResourceInfos)
{
    ResourceInfo srcResourceInfo{};

    for (const auto& info : commandResourceInfos)
    {
        for (const auto& [buffer, _] : info.dst.buffers)
        {
            if (findSrcBuffer(submittedCommandBuffers, buffer))
            {
                srcResourceInfo.buffers[buffer] = getSrcBufferUsageInfo(submittedCommandBuffers, buffer);
            }
        }

        for (const auto& [texture, _] : info.src.textures)
        {
            if (findSrcTexture(submittedCommandBuffers, texture))
            {
                srcResourceInfo.textures[texture] = getSrcTextureUsageInfo(submittedCommandBuffers, texture);
            }
        }
    }

    return srcResourceInfo;
};

std::vector<VkSemaphore> getSrcBufferSemaphores(std::vector<VulkanSubmit>& submits, Buffer* buffer)
{
    std::vector<VkSemaphore> semaphores{};
    for (auto& submit : submits)
    {
        if (submit.object.srcResource.buffers.contains(downcast(buffer)->getVkBuffer()))
        {
            if (submit.info.signalSemaphores.empty())
            {
                auto device = downcast(buffer)->getDevice();
                auto semaphore = device->getSemaphorePool()->create();
                submit.addSignalSemaphore({ semaphore });

                device->getInflightObjects()->standby(semaphore);
                device->getDeleter()->safeDestroy(semaphore);
            }
            semaphores.insert(semaphores.end(), submit.info.signalSemaphores.begin(), submit.info.signalSemaphores.end());
        }
    }

    return semaphores;
};

std::vector<VkSemaphore> getSrcTextureSemaphores(std::vector<VulkanSubmit>& submits, Texture* texture)
{
    std::vector<VkSemaphore> semaphores{};
    for (auto& submit : submits)
    {
        if (submit.object.srcResource.images.contains(downcast(texture)->getVkImage()))
        {
            if (submit.info.signalSemaphores.empty())
            {
                auto device = downcast(texture)->getDevice();
                auto semaphore = device->getSemaphorePool()->create();
                submit.addSignalSemaphore({ semaphore });

                device->getInflightObjects()->standby(semaphore);
                device->getDeleter()->safeDestroy(semaphore);
            }
            semaphores.insert(semaphores.end(), submit.info.signalSemaphores.begin(), submit.info.signalSemaphores.end());
        }
    }

    return semaphores;
};

SubmitType getSubmitType(VulkanCommandBuffer* commandBuffer)
{
    const auto& commandResourceInfos = commandBuffer->getCommandResourceInfos();
    if (commandResourceInfos.empty())
    {
        // assumed copied resources.
        return SubmitType::kTransfer;
    }
    const auto& info = commandResourceInfos[commandResourceInfos.size() - 1]; // only last operation resource info

    const auto& src = info.src;

    if (src.buffers.empty() && src.textures.empty())
    {
        spdlog::error("There is no output resource. return kNone submit type.");
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
                return SubmitType::kGraphics;
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

    spdlog::error("Return kNone submit type.");

    return SubmitType::kNone;
};

VulkanSubmit getDefaultSubmit(VulkanDevice* device)
{
    return VulkanSubmit{};
};

VulkanSubmitContext VulkanSubmitContext::create(VulkanDevice* device, const std::vector<CommandBuffer*>& commandBuffers)
{
    VulkanSubmitContext context{};

    VulkanSubmit currentSubmit = getDefaultSubmit(device);
    std::vector<VulkanCommandBuffer*> submittedCommandBuffers{};
    for (const auto commandBuffer : commandBuffers)
    {
        auto vulkanCommandBuffer = downcast(commandBuffer);
        // generate submit info
        {
            auto hasSrcDependency = findSrcResource(submittedCommandBuffers, vulkanCommandBuffer->getCommandResourceInfos());

            if (hasSrcDependency)
            {
                // prepare next submit info.
                // because we need to wait for the previous submit by semaphore.
                context.m_submits.push_back(currentSubmit);
                currentSubmit = getDefaultSubmit(device);
            }

            // set command buffer
            {
                currentSubmit.add(vulkanCommandBuffer->getVkCommandBuffer());
            }

            // set wait semaphore
            {
                if (hasSrcDependency) // set wait semaphore and resource info
                {
                    std::vector<VkPipelineStageFlags> waitStages{};
                    std::vector<VkSemaphore> waitSemaphores{};

                    const auto& commandResourceInfos = vulkanCommandBuffer->getCommandResourceInfos();

                    for (const auto& commandResourceInfo : commandResourceInfos)
                    {
                        for (const auto& [dstBuffer, dstBufferUsageInfo] : commandResourceInfo.dst.buffers)
                        {
                            if (findSrcBuffer(submittedCommandBuffers, dstBuffer))
                            {
                                auto bufferWaitSemaphores = getSrcBufferSemaphores(context.m_submits, dstBuffer);
                                waitSemaphores.insert(waitSemaphores.end(), bufferWaitSemaphores.begin(), bufferWaitSemaphores.end());

                                waitStages.push_back(dstBufferUsageInfo.stageFlags);
                            }
                        }

                        for (const auto& [dstTexture, dstTextureUsageInfo] : commandResourceInfo.dst.textures)
                        {
                            if (findSrcTexture(submittedCommandBuffers, dstTexture))
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
                    if (getSubmitType(vulkanCommandBuffer) == SubmitType::kPresent)
                    {
                        auto& commandResourceInfos = vulkanCommandBuffer->getCommandResourceInfos();
                        for (const auto& commandResourceInfo : commandResourceInfos)
                        {
                            for (const auto& [srcTexture, _] : commandResourceInfo.src.textures)
                            {
                                VulkanTexture* vulkanTexture = downcast(srcTexture);
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
                currentSubmit.info.type = getSubmitType(vulkanCommandBuffer);
            }
        }

        // generate submit objects
        {
            for (auto& command : vulkanCommandBuffer->getCommands())
            {
                switch (command->type)
                {
                case CommandType::kCopyBufferToBuffer:
                    currentSubmit.add(reinterpret_cast<CopyBufferToBufferCommand*>(command.get()));
                    break;
                case CommandType::kCopyBufferToTexture:
                    currentSubmit.add(reinterpret_cast<CopyBufferToTextureCommand*>(command.get()));
                    break;
                case CommandType::kCopyTextureToBuffer:
                    currentSubmit.add(reinterpret_cast<CopyTextureToBufferCommand*>(command.get()));
                    break;
                case CommandType::kCopyTextureToTexture:
                    currentSubmit.add(reinterpret_cast<CopyTextureToTextureCommand*>(command.get()));
                    break;
                case CommandType::kBeginComputePass:
                case CommandType::kEndComputePass:
                case CommandType::kDispatch:
                case CommandType::kDispatchIndirect:
                    // do nothing.
                    break;
                case CommandType::kSetComputePipeline:
                    currentSubmit.add(reinterpret_cast<SetComputePipelineCommand*>(command.get()));
                    break;
                case CommandType::kSetComputeBindGroup:
                    currentSubmit.addComputeBindGroup(reinterpret_cast<SetBindGroupCommand*>(command.get()));
                    break;
                case CommandType::kBeginRenderPass:
                    currentSubmit.add(reinterpret_cast<BeginRenderPassCommand*>(command.get()));
                    break;
                case CommandType::kSetRenderPipeline:
                    currentSubmit.add(reinterpret_cast<SetRenderPipelineCommand*>(command.get()));
                    break;
                case CommandType::kSetRenderBindGroup:
                    currentSubmit.addRenderBindGroup(reinterpret_cast<SetBindGroupCommand*>(command.get()));
                    break;
                case CommandType::kSetIndexBuffer:
                    currentSubmit.add(reinterpret_cast<SetIndexBufferCommand*>(command.get()));
                    break;
                case CommandType::kSetVertexBuffer:
                    currentSubmit.add(reinterpret_cast<SetVertexBufferCommand*>(command.get()));
                    break;
                case CommandType::kExecuteBundle:
                    currentSubmit.add(reinterpret_cast<ExecuteBundleCommand*>(command.get()));
                    break;
                default:
                    // do nothing.
                    break;
                }
            }
        }

        submittedCommandBuffers.push_back(vulkanCommandBuffer);
    }

    context.m_submits.push_back(currentSubmit);

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