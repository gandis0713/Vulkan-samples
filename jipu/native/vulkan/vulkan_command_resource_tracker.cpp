#include "vulkan_command_resource_tracker.h"

#include "vulkan_binding_group.h"
#include "vulkan_binding_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_command.h"
#include "vulkan_framebuffer.h"
#include "vulkan_render_pass.h"
#include "vulkan_texture.h"

namespace jipu
{

void VulkanCommandResourceTracker::beginComputePass(BeginComputePassCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::setComputePipeline(SetComputePipelineCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::setComputeBindingGroup(SetBindGroupCommand* command)
{
    // dst
    if (false) // TODO
    {
        auto bufferBindings = command->bindingGroup->getBufferBindings();
        for (auto& bufferBinding : bufferBindings)
        {
            m_ongoingPassResourceInfo.dst.buffers[bufferBinding.buffer] = BufferUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_READ_BIT,
            };
        }

        auto textureBindings = command->bindingGroup->getTextureBindings();
        for (auto& textureBinding : textureBindings)
        {
            m_ongoingPassResourceInfo.dst.textures[textureBinding.textureView->getTexture()] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                .layout = VK_IMAGE_LAYOUT_GENERAL,
            };
        }
    }

    // src
    {
        auto bufferBindings = command->bindingGroup->getBufferBindings();
        for (auto& bufferBinding : bufferBindings)
        {
            m_ongoingPassResourceInfo.src.buffers[bufferBinding.buffer] = BufferUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_WRITE_BIT,
            };
        }

        auto textureBindings = command->bindingGroup->getTextureBindings();
        for (auto& textureBinding : textureBindings)
        {
            m_ongoingPassResourceInfo.src.textures[textureBinding.textureView->getTexture()] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_WRITE_BIT,
                .layout = VK_IMAGE_LAYOUT_GENERAL,
            };
        }
    }
}

void VulkanCommandResourceTracker::dispatch(DispatchCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::dispatchIndirect(DispatchIndirectCommand* command)
{
    // TODO
}

void VulkanCommandResourceTracker::endComputePass(EndComputePassCommand* command)
{
    m_passResourceInfos.push_back(std::move(m_ongoingPassResourceInfo));
    m_ongoingPassResourceInfo = {};
}

void VulkanCommandResourceTracker::beginRenderPass(BeginRenderPassCommand* command)
{
    // src
    {
        auto& framebuffer = command->framebuffer;
        auto& renderPass = command->renderPass;

        const auto& framebufferColorAttachments = framebuffer->getColorAttachments();
        const auto& renderPassColorAttachments = renderPass->getColorAttachments();

        for (auto i = 0; i < framebufferColorAttachments.size(); ++i)
        {
            const auto& framebufferColorAttachment = framebufferColorAttachments[i];
            const auto& renderPassColorAttachment = renderPassColorAttachments[i];

            if (framebufferColorAttachment.resolveView)
            {
                m_ongoingPassResourceInfo.dst.textures[framebufferColorAttachment.resolveView->getTexture()] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .accessFlags = VK_ACCESS_NONE,
                    .layout = renderPassColorAttachment.resolveAttachment.value().initialLayout,
                };

                m_ongoingPassResourceInfo.src.textures[framebufferColorAttachment.resolveView->getTexture()] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .layout = renderPassColorAttachment.resolveAttachment.value().finalLayout,
                };
            }
            else
            {
                m_ongoingPassResourceInfo.dst.textures[framebufferColorAttachment.renderView->getTexture()] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .accessFlags = VK_ACCESS_NONE,
                    .layout = renderPassColorAttachment.renderAttachment.initialLayout,
                };

                m_ongoingPassResourceInfo.src.textures[framebufferColorAttachment.renderView->getTexture()] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .layout = renderPassColorAttachment.renderAttachment.finalLayout,
                };
            }
        }
    }
}

void VulkanCommandResourceTracker::setRenderPipeline(SetRenderPipelineCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::setVertexBuffer(SetVertexBufferCommand* command)
{
    // dst
    {
        m_ongoingPassResourceInfo.dst.buffers[command->buffer] = BufferUsageInfo{
            .stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            .accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        };
    }
}

void VulkanCommandResourceTracker::setIndexBuffer(SetIndexBufferCommand* command)
{
    // dst
    {
        m_ongoingPassResourceInfo.dst.buffers[command->buffer] = BufferUsageInfo{
            .stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            .accessFlags = VK_ACCESS_INDEX_READ_BIT,
        };
    }
}

void VulkanCommandResourceTracker::setViewport(SetViewportCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::setScissor(SetScissorCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::setBlendConstant(SetBlendConstantCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::draw(DrawCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::drawIndexed(DrawIndexedCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::beginOcclusionQuery(BeginOcclusionQueryCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::endOcclusionQuery(EndOcclusionQueryCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::endRenderPass(EndRenderPassCommand* command)
{
    m_passResourceInfos.push_back(std::move(m_ongoingPassResourceInfo));
    m_ongoingPassResourceInfo = {};
}

void VulkanCommandResourceTracker::setRenderBindingGroup(SetBindGroupCommand* command)
{
    // dst
    {
        auto bindingGroup = command->bindingGroup;
        auto bindingGroupLayout = command->bindingGroup->getLayout();

        auto bufferBindings = bindingGroup->getBufferBindings();
        auto bufferBindingLayouts = bindingGroupLayout->getBufferBindingLayouts();
        for (auto i = 0; i < bufferBindings.size(); ++i)
        {
            auto& bufferBinding = bufferBindings[i];
            auto& bufferBindingLayout = bufferBindingLayouts[i];

            auto bufferUsageInfo = BufferUsageInfo{ .stageFlags = VK_PIPELINE_STAGE_NONE,
                                                    .accessFlags = VK_ACCESS_NONE };

            if (bufferBindingLayout.stages & BindingStageFlagBits::kComputeStage)
            {
                bufferUsageInfo.stageFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (bufferBindingLayout.stages & BindingStageFlagBits::kVertexStage)
            {
                bufferUsageInfo.stageFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            }
            if (bufferBindingLayout.stages & BindingStageFlagBits::kFragmentStage)
            {
                bufferUsageInfo.stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }

            switch (bufferBindingLayout.type)
            {
            case BufferBindingType::kUniform:
                bufferUsageInfo.accessFlags |= VK_ACCESS_UNIFORM_READ_BIT;
                break;
            case BufferBindingType::kStorage:
                bufferUsageInfo.accessFlags |= VK_ACCESS_SHADER_WRITE_BIT;
                break;
            case BufferBindingType::kReadOnlyStorage:
                bufferUsageInfo.accessFlags |= VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                bufferUsageInfo.accessFlags |= VK_ACCESS_SHADER_READ_BIT;
                break;
            }
        }

        auto textureBindings = bindingGroup->getTextureBindings();
        auto textureBindingLayouts = bindingGroupLayout->getTextureBindingLayouts();
        for (auto i = 0; i < textureBindings.size(); ++i)
        {
            auto& textureBinding = textureBindings[i];
            auto& textureBindingLayout = textureBindingLayouts[i];

            auto textureUsageInfo = TextureUsageInfo{ .stageFlags = VK_PIPELINE_STAGE_NONE,
                                                      .accessFlags = VK_ACCESS_NONE,
                                                      .layout = VK_IMAGE_LAYOUT_UNDEFINED };

            if (textureBindingLayout.stages & BindingStageFlagBits::kComputeStage)
            {
                textureUsageInfo.stageFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (textureBindingLayout.stages & BindingStageFlagBits::kVertexStage)
            {
                textureUsageInfo.stageFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            }
            if (textureBindingLayout.stages & BindingStageFlagBits::kFragmentStage)
            {
                textureUsageInfo.stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            textureUsageInfo.accessFlags |= VK_ACCESS_SHADER_READ_BIT;
            textureUsageInfo.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    // src
    {
        // TODO
    }
}

void VulkanCommandResourceTracker::copyBufferToBuffer(CopyBufferToBufferCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::copyBufferToTexture(CopyBufferToTextureCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::copyTextureToBuffer(CopyTextureToBufferCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::copyTextureToTexture(CopyTextureToTextureCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::resolveQuerySet(ResolveQuerySetCommand* command)
{
    // do nothing.
}

std::vector<PassResourceInfo> VulkanCommandResourceTracker::result()
{
    return m_passResourceInfos;
}

} // namespace jipu