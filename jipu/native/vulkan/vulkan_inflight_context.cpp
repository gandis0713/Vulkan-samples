#include "vulkan_inflight_context.h"

#include "vulkan_binding_group.h"
#include "vulkan_binding_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_pipeline_layout.h"
#include "vulkan_render_pass.h"
#include "vulkan_sampler.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

namespace jipu
{

VulkanInflightObject VulkanInflightContext::generate(std::vector<CommandBuffer*> commandBuffers)
{
    VulkanInflightObject inflightObject{};
    for (auto& commandBuffer : commandBuffers)
    {
        auto vulkanCommandBuffer = downcast(commandBuffer);
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
                inflightObject.pipelines.insert(downcast(cmd->pipeline)->getVkPipeline());
                inflightObject.pipelineLayouts.insert(downcast(cmd->pipeline->getPipelineLayout())->getVkPipelineLayout());
            }
            break;
            case CommandType::kSetComputeBindGroup: {
                auto cmd = reinterpret_cast<SetBindGroupCommand*>(command.get());
                inflightObject.descriptorSetLayouts.insert(downcast(cmd->bindingGroup->getLayout())->getVkDescriptorSetLayout());
                for (auto& binding : cmd->bindingGroup->getBufferBindings())
                {
                    inflightObject.buffers.insert(downcast(binding.buffer)->getVkBuffer());
                }
                for (auto& binding : cmd->bindingGroup->getSmaplerBindings())
                {
                    inflightObject.samplers.insert(downcast(binding.sampler)->getVkSampler());
                }
                for (auto& binding : cmd->bindingGroup->getTextureBindings())
                {
                    inflightObject.images.insert(downcast(binding.textureView->getTexture())->getVkImage());
                    inflightObject.imageViews.insert(downcast(binding.textureView)->getVkImageView());
                }
            }
            break;
            case CommandType::kBeginRenderPass: {
                auto cmd = reinterpret_cast<BeginRenderPassCommand*>(command.get());
                for (auto& colorAttachment : cmd->framebuffer->getColorAttachments())
                {
                    inflightObject.images.insert(downcast(colorAttachment.renderView->getTexture())->getVkImage());
                    inflightObject.imageViews.insert(downcast(colorAttachment.renderView)->getVkImageView());
                    if (colorAttachment.resolveView)
                    {
                        inflightObject.images.insert(downcast(colorAttachment.resolveView->getTexture())->getVkImage());
                        inflightObject.imageViews.insert(downcast(colorAttachment.resolveView)->getVkImageView());
                    }
                }
                inflightObject.renderPasses.insert(cmd->renderPass->getVkRenderPass());
            }
            break;
            case CommandType::kSetRenderPipeline: {
                auto cmd = reinterpret_cast<SetRenderPipelineCommand*>(command.get());
                inflightObject.pipelines.insert(downcast(cmd->pipeline)->getVkPipeline());
                inflightObject.pipelineLayouts.insert(downcast(cmd->pipeline->getPipelineLayout())->getVkPipelineLayout());
            }
            break;
            case CommandType::kSetRenderBindGroup: {
                auto cmd = reinterpret_cast<SetBindGroupCommand*>(command.get());
                inflightObject.descriptorSetLayouts.insert(downcast(cmd->bindingGroup->getLayout())->getVkDescriptorSetLayout());
                for (auto& binding : cmd->bindingGroup->getBufferBindings())
                {
                    inflightObject.buffers.insert(downcast(binding.buffer)->getVkBuffer());
                }
                for (auto& binding : cmd->bindingGroup->getSmaplerBindings())
                {
                    inflightObject.samplers.insert(downcast(binding.sampler)->getVkSampler());
                }
                for (auto& binding : cmd->bindingGroup->getTextureBindings())
                {
                    inflightObject.images.insert(downcast(binding.textureView->getTexture())->getVkImage());
                    inflightObject.imageViews.insert(downcast(binding.textureView)->getVkImageView());
                }
            }
            break;
            case CommandType::kSetIndexBuffer: {
                auto cmd = reinterpret_cast<SetIndexBufferCommand*>(command.get());
                inflightObject.buffers.insert(downcast(cmd->buffer)->getVkBuffer());
            }
            break;
            case CommandType::kSetVertexBuffer: {
                auto cmd = reinterpret_cast<SetVertexBufferCommand*>(command.get());
                inflightObject.buffers.insert(downcast(cmd->buffer)->getVkBuffer());
            }
            break;
            }
        }
    }

    return inflightObject;
}

VulkanInflightContext::VulkanInflightContext(VulkanDevice* device)
    : m_device(device)
{
}

VulkanInflightContext::~VulkanInflightContext()
{
    m_inflights.clear();
}

void VulkanInflightContext::add(VkQueue queue, const VulkanSubmit& submit, VkFence fence)
{
    auto& inflight = m_inflights[queue];
    auto& inflightObject = inflight[fence];

    inflightObject.commandBuffers.insert(submit.info.commandBuffers.begin(), submit.info.commandBuffers.end());
    inflightObject.signalSemaphores.insert(submit.info.signalSemaphores.begin(), submit.info.signalSemaphores.end());
    // do not need to wait for semaphores.

    inflightObject.imageViews.insert(submit.object.imageViews.begin(), submit.object.imageViews.end());
    inflightObject.samplers.insert(submit.object.samplers.begin(), submit.object.samplers.end());
    inflightObject.pipelines.insert(submit.object.pipelines.begin(), submit.object.pipelines.end());
    inflightObject.pipelineLayouts.insert(submit.object.pipelineLayouts.begin(), submit.object.pipelineLayouts.end());
    inflightObject.descriptorSet.insert(submit.object.descriptorSet.begin(), submit.object.descriptorSet.end());
    inflightObject.descriptorSetLayouts.insert(submit.object.descriptorSetLayouts.begin(), submit.object.descriptorSetLayouts.end());
    inflightObject.framebuffers.insert(submit.object.framebuffers.begin(), submit.object.framebuffers.end());
    inflightObject.renderPasses.insert(submit.object.renderPasses.begin(), submit.object.renderPasses.end());

    inflightObject.buffers.insert(submit.object.srcResource.buffers.begin(), submit.object.srcResource.buffers.end());
    inflightObject.buffers.insert(submit.object.dstResource.buffers.begin(), submit.object.dstResource.buffers.end());

    inflightObject.images.insert(submit.object.srcResource.images.begin(), submit.object.srcResource.images.end());
    inflightObject.images.insert(submit.object.dstResource.images.begin(), submit.object.dstResource.images.end());
}

bool VulkanInflightContext::clear(VkFence fence)
{
    for (auto& [_, inflight] : m_inflights)
    {
        if (inflight.contains(fence))
        {
            inflight.erase(fence);
            return true;
        }
    }

    return false;
}

bool VulkanInflightContext::clear(VkQueue queue)
{
    if (m_inflights.contains(queue))
    {
        m_inflights.erase(queue);
        return true;
    }

    return false;
}

void VulkanInflightContext::clearAll()
{
}

} // namespace jipu