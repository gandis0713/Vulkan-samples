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
    auto vulkanBindGroup = downcast(command->bindGroup);
    // dst (read)
    {
        // buffer
        {
            auto bufferBindings = vulkanBindGroup->getBufferBindings();
            auto bufferBindingLayouts = vulkanBindGroup->getBufferLayouts();
            for (const auto& bufferBindingLayout : bufferBindingLayouts)
            {
                auto it = std::find_if(bufferBindings.begin(), bufferBindings.end(), [&](const auto& bufferBinding) {
                    return bufferBinding.index == bufferBindingLayout.index;
                });

                if (it == bufferBindings.end())
                    throw std::runtime_error("The buffer binding layout is not found in the buffer bindings.");

                const auto& bufferBinding = *it;

                VkAccessFlags accessFlags = VK_ACCESS_NONE;
                if (bufferBindingLayout.type == BufferBindingType::kUniform)
                {
                    accessFlags |= VK_ACCESS_UNIFORM_READ_BIT;
                }
                if (bufferBindingLayout.type == BufferBindingType::kReadOnlyStorage)
                {
                    accessFlags |= VK_ACCESS_SHADER_READ_BIT;
                }

                m_currentOperationResourceInfo.dst.buffers[bufferBinding.buffer] = BufferUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    .accessFlags = accessFlags,
                };
            }
        }

        // texture and storage texture
        {
            auto textureBindings = vulkanBindGroup->getTextureBindings();

            // texture
            auto textureBindingLayouts = vulkanBindGroup->getTextureLayouts();
            for (const auto& textureBindingLayout : textureBindingLayouts)
            {
                auto it = std::find_if(textureBindings.begin(), textureBindings.end(), [&](const auto& textureBinding) {
                    return textureBinding.index == textureBindingLayout.index;
                });

                if (it == textureBindings.end())
                    throw std::runtime_error("The texture binding layout is not found in the texture bindings.");

                const auto& textureBinding = *it;
                auto vulkanTextureView = downcast(textureBinding.textureView);
                m_currentOperationResourceInfo.dst.textureViews[vulkanTextureView] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                    .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .baseMipLevel = vulkanTextureView->getBaseMipLevel(),
                    .mipLevelCount = vulkanTextureView->getMipLevelCount(),
                    .baseArrayLayer = vulkanTextureView->getBaseArrayLayer(),
                    .arrayLayerCount = vulkanTextureView->getArrayLayerCount(),
                };
            }

            // storage texture
            auto storageTextureLayouts = vulkanBindGroup->getStorageTextureLayouts();
            for (const auto& storageTextureLayout : storageTextureLayouts)
            {
                auto it = std::find_if(textureBindings.begin(), textureBindings.end(), [&](const auto& textureBinding) {
                    return textureBinding.index == storageTextureLayout.index;
                });

                if (it == textureBindings.end())
                    throw std::runtime_error("The texture binding layout is not found in the storage texture bindings.");

                const auto& textureBinding = *it;
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
        }
    }

    // src (write)
    {
        // buffer
        {
            auto bufferBindings = vulkanBindGroup->getBufferBindings();
            auto bufferBindingLayouts = vulkanBindGroup->getBufferLayouts();
            for (const auto& bufferBindingLayout : bufferBindingLayouts)
            {
                auto it = std::find_if(bufferBindings.begin(), bufferBindings.end(), [&](const auto& bufferBinding) {
                    return bufferBinding.index == bufferBindingLayout.index;
                });

                if (it == bufferBindings.end())
                    throw std::runtime_error("The buffer binding layout is not found in the buffer bindings.");

                const auto& bufferBinding = *it;

                VkAccessFlags accessFlags = VK_ACCESS_NONE;
                if (bufferBindingLayout.type == BufferBindingType::kStorage)
                {
                    accessFlags |= VK_ACCESS_SHADER_READ_BIT;
                    accessFlags |= VK_ACCESS_SHADER_WRITE_BIT;
                }

                m_currentOperationResourceInfo.src.buffers[bufferBinding.buffer] = BufferUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    .accessFlags = accessFlags,
                };
            }
        }

        // texture and storage texture
        {
            auto textureBindings = vulkanBindGroup->getTextureBindings();

            // doesn't need to collect texture for read only.
            // storage texture is read and write.
            auto storageTextureLayouts = vulkanBindGroup->getStorageTextureLayouts();
            for (const auto& storageTextureLayout : storageTextureLayouts)
            {
                auto it = std::find_if(textureBindings.begin(), textureBindings.end(), [&](const auto& textureBinding) {
                    return textureBinding.index == storageTextureLayout.index;
                });

                if (it == textureBindings.end())
                    throw std::runtime_error("The texture binding layout is not found in the storage texture bindings.");

                const auto& textureBinding = *it;
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
    m_operationResourceInfos.push_back(std::move(m_currentOperationResourceInfo));
    m_currentOperationResourceInfo = {};
}

void VulkanCommandResourceTracker::beginRenderPass(BeginRenderPassCommand* command)
{
    const auto& colorAttachments = command->colorAttachments;
    const auto& depthStencilAttachment = command->depthStencilAttachment;

    // dst (wait only)
    {
        for (const auto& colorAttachment : colorAttachments)
        {
            auto vulkanRenderTextureView = downcast(colorAttachment.renderView);
            auto vulkanRenderTexture = downcast(vulkanRenderTextureView->getTexture());
            m_currentOperationResourceInfo.dst.textureViews[vulkanRenderTextureView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                // because it is for only wait previous pass result. So, we assume don't need layout for read.
                .layout = vulkanRenderTexture->getOwner() == VulkanTextureOwner::kSwapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .baseMipLevel = vulkanRenderTextureView->getBaseMipLevel(),
                .mipLevelCount = vulkanRenderTextureView->getMipLevelCount(),
                .baseArrayLayer = vulkanRenderTextureView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanRenderTextureView->getArrayLayerCount(),
            };

            if (colorAttachment.resolveView)
            {
                auto vulkanResolveTextureView = downcast(colorAttachment.resolveView);
                auto vulkanResolveTexture = downcast(vulkanResolveTextureView->getTexture());
                m_currentOperationResourceInfo.dst.textureViews[vulkanResolveTextureView] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                    // because it is for only wait previous pass result. So, we assume don't need layout for read.
                    .layout = vulkanResolveTexture->getOwner() == VulkanTextureOwner::kSwapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .baseMipLevel = vulkanResolveTextureView->getBaseMipLevel(),
                    .mipLevelCount = vulkanResolveTextureView->getMipLevelCount(),
                    .baseArrayLayer = vulkanResolveTextureView->getBaseArrayLayer(),
                    .arrayLayerCount = vulkanResolveTextureView->getArrayLayerCount(),
                };
            }
        }

        if (depthStencilAttachment.has_value())
        {
            auto vulkanTextureView = downcast(depthStencilAttachment.value().textureView);
            auto vulkanTexture = downcast(vulkanTextureView->getTexture());
            m_currentOperationResourceInfo.dst.textureViews[vulkanTextureView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .accessFlags = VK_ACCESS_SHADER_READ_BIT,
                // because it is for only wait previous pass result. So, we assume don't need layout for read.
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .baseMipLevel = vulkanTextureView->getBaseMipLevel(),
                .mipLevelCount = vulkanTextureView->getMipLevelCount(),
                .baseArrayLayer = vulkanTextureView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanTextureView->getArrayLayerCount(),
            };
        }
    }

    // src (write)
    {
        for (const auto& colorAttachment : colorAttachments)
        {
            auto vulkanRenderTextureView = downcast(colorAttachment.renderView);
            auto vulkanRenderTexture = downcast(vulkanRenderTextureView->getTexture());
            m_currentOperationResourceInfo.src.textureViews[vulkanRenderTextureView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .layout = vulkanRenderTexture->getOwner() == VulkanTextureOwner::kSwapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .baseMipLevel = vulkanRenderTextureView->getBaseMipLevel(),
                .mipLevelCount = vulkanRenderTextureView->getMipLevelCount(),
                .baseArrayLayer = vulkanRenderTextureView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanRenderTextureView->getArrayLayerCount(),
            };

            if (colorAttachment.resolveView)
            {
                auto vulkanResolveTextureView = downcast(colorAttachment.resolveView);
                auto vulkanResolveTexture = downcast(vulkanResolveTextureView->getTexture());
                m_currentOperationResourceInfo.src.textureViews[vulkanResolveTextureView] = TextureUsageInfo{
                    .stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .layout = vulkanResolveTexture->getOwner() == VulkanTextureOwner::kSwapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .baseMipLevel = vulkanResolveTextureView->getBaseMipLevel(),
                    .mipLevelCount = vulkanResolveTextureView->getMipLevelCount(),
                    .baseArrayLayer = vulkanResolveTextureView->getBaseArrayLayer(),
                    .arrayLayerCount = vulkanResolveTextureView->getArrayLayerCount(),
                };
            }
        }

        if (depthStencilAttachment.has_value())
        {
            auto vulkanTextureView = downcast(depthStencilAttachment.value().textureView);
            auto vulkanTexture = downcast(vulkanTextureView->getTexture());
            m_currentOperationResourceInfo.src.textureViews[vulkanTextureView] = TextureUsageInfo{
                .stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, // | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                .accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .baseMipLevel = vulkanTextureView->getBaseMipLevel(),
                .mipLevelCount = vulkanTextureView->getMipLevelCount(),
                .baseArrayLayer = vulkanTextureView->getBaseArrayLayer(),
                .arrayLayerCount = vulkanTextureView->getArrayLayerCount(),
            };
        }
    }
}

void VulkanCommandResourceTracker::setRenderPipeline(SetRenderPipelineCommand* command)
{
    // do nothing.
}

void VulkanCommandResourceTracker::setVertexBuffer(SetVertexBufferCommand* command)
{
    // dst (read)
    {
        m_currentOperationResourceInfo.dst.buffers[command->buffer] = BufferUsageInfo{
            .stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            .accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        };
    }
}

void VulkanCommandResourceTracker::setIndexBuffer(SetIndexBufferCommand* command)
{
    // dst (read)
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
    // dst (read)
    {
        auto bindGroup = downcast(command->bindGroup);

        // buffer
        {
            auto bufferBindings = bindGroup->getBufferBindings();
            auto bufferBindingLayouts = bindGroup->getBufferLayouts();

            for (const auto& bufferBindingLayout : bufferBindingLayouts)
            {
                auto it = std::find_if(bufferBindings.begin(), bufferBindings.end(), [&](const auto& bufferBinding) {
                    return bufferBinding.index == bufferBindingLayout.index;
                });

                if (it == bufferBindings.end())
                    throw std::runtime_error("The buffer binding layout is not found in the buffer bindings.");

                const auto& bufferBinding = *it;

                auto bufferUsageInfo = BufferUsageInfo{ .stageFlags = VK_PIPELINE_STAGE_NONE,
                                                        .accessFlags = VK_ACCESS_NONE };

                if (bufferBindingLayout.stages & BindingStageFlagBits::kVertexStage)
                {
                    bufferUsageInfo.stageFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                }
                if (bufferBindingLayout.stages & BindingStageFlagBits::kFragmentStage)
                {
                    bufferUsageInfo.stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }

                if (bufferBindingLayout.type == BufferBindingType::kUniform)
                {
                    bufferUsageInfo.accessFlags |= VK_ACCESS_UNIFORM_READ_BIT;
                }
                if (bufferBindingLayout.type == BufferBindingType::kReadOnlyStorage)
                {
                    bufferUsageInfo.accessFlags |= VK_ACCESS_SHADER_READ_BIT;
                }
                // TODO: consider storage buffer

                m_currentOperationResourceInfo.dst.buffers[bufferBinding.buffer] = bufferUsageInfo;
            }
        }

        // texture and storage texture
        {
            auto textureBindings = bindGroup->getTextureBindings();

            // texture
            auto textureBindingLayouts = bindGroup->getTextureLayouts();
            for (const auto& textureBindingLayout : textureBindingLayouts)
            {
                auto it = std::find_if(textureBindings.begin(), textureBindings.end(), [&](const auto& textureBinding) {
                    return textureBinding.index == textureBindingLayout.index;
                });

                if (it == textureBindings.end())
                    throw std::runtime_error("The texture binding layout is not found in the texture bindings.");

                const auto& textureBinding = *it;

                auto textureUsageInfo = TextureUsageInfo{ .stageFlags = VK_PIPELINE_STAGE_NONE,
                                                          .accessFlags = VK_ACCESS_NONE,
                                                          .layout = VK_IMAGE_LAYOUT_UNDEFINED };

                if (textureBindingLayout.stages & BindingStageFlagBits::kVertexStage)
                {
                    textureUsageInfo.stageFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                }
                if (textureBindingLayout.stages & BindingStageFlagBits::kFragmentStage)
                {
                    textureUsageInfo.stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                }

                textureUsageInfo.accessFlags |= VK_ACCESS_SHADER_READ_BIT;

                auto vulkanTextureView = downcast(textureBinding.textureView);
                auto vulkanTexture = downcast(vulkanTextureView->getTexture());

                textureUsageInfo.layout = vulkanTexture->isDepthStencil() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                textureUsageInfo.baseMipLevel = vulkanTextureView->getBaseMipLevel();
                textureUsageInfo.mipLevelCount = vulkanTextureView->getMipLevelCount();
                textureUsageInfo.baseArrayLayer = vulkanTextureView->getBaseArrayLayer();
                textureUsageInfo.arrayLayerCount = vulkanTextureView->getArrayLayerCount();

                m_currentOperationResourceInfo.dst.textureViews[vulkanTextureView] = textureUsageInfo;
            }

            // TODO: consider storage texture
        }
    }

    // src (write)
    {
        // do nothing
    }
}

void VulkanCommandResourceTracker::copyBufferToBuffer(CopyBufferToBufferCommand* command)
{
    // TODO
}

void VulkanCommandResourceTracker::copyBufferToTexture(CopyBufferToTextureCommand* command)
{
    // TODO
}

void VulkanCommandResourceTracker::copyTextureToBuffer(CopyTextureToBufferCommand* command)
{
    // TODO
}

void VulkanCommandResourceTracker::copyTextureToTexture(CopyTextureToTextureCommand* command)
{
    // TODO
}

void VulkanCommandResourceTracker::resolveQuerySet(ResolveQuerySetCommand* command)
{
    // TODO
}

VulkanResourceTrackingResult VulkanCommandResourceTracker::finish()
{
    m_currentOperationResourceInfo = {};

    return VulkanResourceTrackingResult{ .operationResourceInfos = std::move(m_operationResourceInfos) };
}

} // namespace jipu