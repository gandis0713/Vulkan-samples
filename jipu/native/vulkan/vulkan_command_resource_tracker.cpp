#include "vulkan_command_resource_tracker.h"

#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_command.h"
#include "vulkan_framebuffer.h"
#include "vulkan_render_bundle.h"
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

void VulkanCommandResourceTracker::setComputeBindGroup(SetBindGroupCommand* command)
{
    // dst
    // if (false) // TODO
    {
        auto bufferBindings = command->bindGroup->getBufferBindings();
        for (auto& bufferBinding : bufferBindings)
        {
            m_currentOperationResourceInfo.dst.buffers[bufferBinding.buffer] = BufferUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_READ_BIT,
            };
        }

        auto textureBindings = command->bindGroup->getTextureBindings();
        for (auto& textureBinding : textureBindings)
        {
            auto vulkanTextureView = downcast(textureBinding.textureView);
            m_currentOperationResourceInfo.dst.textureViews[vulkanTextureView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                .layout = VK_IMAGE_LAYOUT_GENERAL,
                .baseMipLevel = vulkanTextureView->getBaseMipLevel(),
                .mipLevelCount = vulkanTextureView->getMipLevelCount(),
                .baseArrayLayer = vulkanTextureView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanTextureView->getArrayLayerCount(),
            };
        }

        // TODO: storage texture
    }

    // src
    {
        auto bufferBindings = command->bindGroup->getBufferBindings();
        for (auto& bufferBinding : bufferBindings)
        {
            m_currentOperationResourceInfo.src.buffers[bufferBinding.buffer] = BufferUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_WRITE_BIT,
            };
        }

        auto textureBindings = command->bindGroup->getTextureBindings();
        for (auto& textureBinding : textureBindings)
        {
            auto vulkanTextureView = downcast(textureBinding.textureView);
            m_currentOperationResourceInfo.src.textureViews[vulkanTextureView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_WRITE_BIT,
                .layout = VK_IMAGE_LAYOUT_GENERAL,
                .baseMipLevel = vulkanTextureView->getBaseMipLevel(),
                .mipLevelCount = vulkanTextureView->getMipLevelCount(),
                .baseArrayLayer = vulkanTextureView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanTextureView->getArrayLayerCount(),
            };
        }

        // TODO: storage texture
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
    m_operationResourceInfos.push_back(std::move(m_currentOperationResourceInfo));
    m_currentOperationResourceInfo = {};
}

void VulkanCommandResourceTracker::beginRenderPass(BeginRenderPassCommand* command)
{
    // src
    {
        auto renderPass = command->renderPass.lock();
        if (!renderPass)
            throw std::runtime_error("The render pass is null for tracking begin render pass command.");

        auto framebuffer = command->framebuffer.lock();
        if (!framebuffer)
            throw std::runtime_error("The framebuffer is null for tracking begin render pass command.");

        const auto& framebufferColorAttachments = framebuffer->getColorAttachments();
        const auto& renderPassColorAttachments = renderPass->getColorAttachments();

        for (auto i = 0; i < framebufferColorAttachments.size(); ++i)
        {
            const auto& framebufferColorAttachment = framebufferColorAttachments[i];
            const auto& renderPassColorAttachment = renderPassColorAttachments[i];

            auto vulkanTextureRenderView = downcast(framebufferColorAttachment.renderView);
            m_currentOperationResourceInfo.dst.textureViews[vulkanTextureRenderView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                .layout = renderPassColorAttachment.renderAttachment.initialLayout,
                .baseMipLevel = vulkanTextureRenderView->getBaseMipLevel(),
                .mipLevelCount = vulkanTextureRenderView->getMipLevelCount(),
                .baseArrayLayer = vulkanTextureRenderView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanTextureRenderView->getArrayLayerCount(),
            };

            m_currentOperationResourceInfo.src.textureViews[vulkanTextureRenderView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .layout = renderPassColorAttachment.renderAttachment.finalLayout,
                .baseMipLevel = vulkanTextureRenderView->getBaseMipLevel(),
                .mipLevelCount = vulkanTextureRenderView->getMipLevelCount(),
                .baseArrayLayer = vulkanTextureRenderView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanTextureRenderView->getArrayLayerCount(),
            };

            if (framebufferColorAttachment.resolveView)
            {
                auto vulkanTextureResolveView = downcast(framebufferColorAttachment.resolveView);
                m_currentOperationResourceInfo.dst.textureViews[vulkanTextureResolveView] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                    .layout = renderPassColorAttachment.resolveAttachment.value().initialLayout,
                    .baseMipLevel = vulkanTextureResolveView->getBaseMipLevel(),
                    .mipLevelCount = vulkanTextureResolveView->getMipLevelCount(),
                    .baseArrayLayer = vulkanTextureResolveView->getBaseArrayLayer(),
                    .arrayLayerCount = vulkanTextureResolveView->getArrayLayerCount(),
                };

                m_currentOperationResourceInfo.src.textureViews[vulkanTextureResolveView] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .layout = renderPassColorAttachment.resolveAttachment.value().finalLayout,
                    .baseMipLevel = vulkanTextureResolveView->getBaseMipLevel(),
                    .mipLevelCount = vulkanTextureResolveView->getMipLevelCount(),
                    .baseArrayLayer = vulkanTextureResolveView->getBaseArrayLayer(),
                    .arrayLayerCount = vulkanTextureResolveView->getArrayLayerCount(),
                };
            }

            // TODO: depth stencil attachment
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
        m_currentOperationResourceInfo.dst.buffers[command->buffer] = BufferUsageInfo{
            .stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            .accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        };
    }
}

void VulkanCommandResourceTracker::setIndexBuffer(SetIndexBufferCommand* command)
{
    // dst
    {
        m_currentOperationResourceInfo.dst.buffers[command->buffer] = BufferUsageInfo{
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

void VulkanCommandResourceTracker::executeBundle(ExecuteBundleCommand* command)
{
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
    m_operationResourceInfos.push_back(std::move(m_currentOperationResourceInfo));
    m_currentOperationResourceInfo = {};
}

void VulkanCommandResourceTracker::setRenderBindGroup(SetBindGroupCommand* command)
{
    // dst
    {
        auto bindGroup = downcast(command->bindGroup);

        auto bufferBindings = bindGroup->getBufferBindings();
        auto bufferBindingLayouts = bindGroup->getBufferLayouts();
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

            m_currentOperationResourceInfo.dst.buffers[bufferBinding.buffer] = bufferUsageInfo;
        }

        auto textureBindings = bindGroup->getTextureBindings();
        auto textureBindingLayouts = bindGroup->getTextureLayouts();
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

            auto vulkanTextureView = downcast(textureBinding.textureView);
            textureUsageInfo.baseMipLevel = vulkanTextureView->getBaseMipLevel();
            textureUsageInfo.mipLevelCount = vulkanTextureView->getMipLevelCount();
            textureUsageInfo.baseArrayLayer = vulkanTextureView->getBaseArrayLayer();
            textureUsageInfo.arrayLayerCount = vulkanTextureView->getArrayLayerCount();

            m_currentOperationResourceInfo.dst.textureViews[vulkanTextureView] = textureUsageInfo;
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

VulkanResourceTrackingResult VulkanCommandResourceTracker::finish()
{
    m_currentOperationResourceInfo = {};

    return VulkanResourceTrackingResult{ .operationResourceInfos = std::move(m_operationResourceInfos) };
}

} // namespace jipu