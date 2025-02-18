
#include "vulkan_render_pass_encoder.h"

#include "vulkan_bind_group.h"
#include "vulkan_buffer.h"
#include "vulkan_command_encoder.h"
#include "vulkan_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_pipeline_layout.h"
#include "vulkan_query_set.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include <optional>
#include <spdlog/spdlog.h>

namespace jipu
{

namespace
{

VkImageLayout generateColorFinalLayout(VulkanTexture* texture)
{
    return texture->getOwner() == VulkanTextureOwner::kSwapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

VkImageLayout generateColorInitialLayout(VulkanTexture* texture, LoadOp loadOp)
{
    auto finalLayout = generateColorFinalLayout(texture);
    return loadOp == LoadOp::kLoad ? finalLayout : texture->getCurrentLayout();
}

VkImageLayout generateDepthStencilFinalLayout(VulkanTexture* texture)
{
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

VkImageLayout generateDepthStencilInitialLayout(VulkanTexture* texture, LoadOp depthLoadOp)
{
    auto finalLayout = generateDepthStencilFinalLayout(texture);
    return depthLoadOp == LoadOp::kLoad ? finalLayout : texture->getCurrentLayout();
}

} // namespace

std::vector<VkClearValue> generateClearColor(const std::vector<ColorAttachment>& colorAttachments,
                                             const std::optional<DepthStencilAttachment>& depthStencilAttachment)
{
    std::vector<VkClearValue> clearValues{};

    auto addColorClearValue = [](std::vector<VkClearValue>& clearValues, const std::vector<ColorAttachment>& colorAttachments) {
        for (auto i = 0; i < colorAttachments.size(); ++i)
        {
            const auto& colorAttachment = colorAttachments[i];
            if (colorAttachment.loadOp == LoadOp::kClear)
            {
                VkClearValue colorClearValue{};
                colorClearValue.color.float32[0] = colorAttachment.clearValue.r;
                colorClearValue.color.float32[1] = colorAttachment.clearValue.g;
                colorClearValue.color.float32[2] = colorAttachment.clearValue.b;
                colorClearValue.color.float32[3] = colorAttachment.clearValue.a;

                clearValues.push_back(colorClearValue);

                if (colorAttachment.resolveView)
                {
                    clearValues.push_back(colorClearValue);
                }
            }
        }
    };

    addColorClearValue(clearValues, colorAttachments);

    if (depthStencilAttachment.has_value())
    {
        auto depthStencil = depthStencilAttachment.value();
        if (depthStencil.depthLoadOp == LoadOp::kClear || depthStencil.stencilLoadOp == LoadOp::kClear)
        {
            VkClearValue depthStencilClearValue{};
            depthStencilClearValue.depthStencil = { depthStencil.clearValue.depth,
                                                    depthStencil.clearValue.stencil };

            clearValues.push_back(depthStencilClearValue);
        }
    }

    return clearValues;
}

VulkanRenderPassDescriptor generateVulkanRenderPassDescriptor(const std::vector<ColorAttachment>& colorAttachments,
                                                              const std::optional<DepthStencilAttachment>& depthStencilAttachment)
{
    if (colorAttachments.empty())
        throw std::runtime_error("Failed to create vulkan render pass encoder due to empty color attachment.");

    VulkanRenderPassDescriptor vkdescriptor{};

    for (const auto& colorAttachment : colorAttachments)
    {
        const auto vulkanRenderTexture = downcast(colorAttachment.renderView->getTexture());

        VkAttachmentDescription renderAttachment{};
        renderAttachment.format = ToVkFormat(vulkanRenderTexture->getFormat());
        renderAttachment.loadOp = ToVkAttachmentLoadOp(colorAttachment.loadOp);
        renderAttachment.storeOp = ToVkAttachmentStoreOp(colorAttachment.storeOp);
        renderAttachment.samples = ToVkSampleCountFlagBits(vulkanRenderTexture->getSampleCount());
        renderAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        renderAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        renderAttachment.initialLayout = generateColorInitialLayout(vulkanRenderTexture, colorAttachment.loadOp);
        renderAttachment.finalLayout = generateColorFinalLayout(vulkanRenderTexture);

        RenderPassColorAttachment renderPassColorAttachment{};
        renderPassColorAttachment.renderAttachment = renderAttachment;

        if (colorAttachment.resolveView)
        {
            const auto vulkanResolveTexture = downcast(colorAttachment.resolveView->getTexture());

            VkAttachmentDescription resolveAttachment{};
            resolveAttachment.format = ToVkFormat(vulkanResolveTexture->getFormat());
            resolveAttachment.loadOp = ToVkAttachmentLoadOp(colorAttachment.loadOp);
            resolveAttachment.storeOp = ToVkAttachmentStoreOp(colorAttachment.storeOp);
            resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // should use VK_SAMPLE_COUNT_1_BIT for resolve attachment.
            resolveAttachment.initialLayout = generateColorInitialLayout(vulkanResolveTexture, colorAttachment.loadOp);
            resolveAttachment.finalLayout = generateColorFinalLayout(vulkanResolveTexture);

            renderPassColorAttachment.resolveAttachment = resolveAttachment;
        }

        vkdescriptor.colorAttachmentDescriptions.push_back(renderPassColorAttachment);
    }

    if (depthStencilAttachment.has_value())
    {
        auto depthStencil = depthStencilAttachment.value();

        const auto vulkanTexture = downcast(depthStencil.textureView->getTexture());

        VkAttachmentDescription attachment{};
        attachment.format = ToVkFormat(vulkanTexture->getFormat());
        attachment.loadOp = ToVkAttachmentLoadOp(depthStencil.depthLoadOp);
        attachment.storeOp = ToVkAttachmentStoreOp(depthStencil.depthStoreOp);
        attachment.stencilLoadOp = ToVkAttachmentLoadOp(depthStencil.stencilLoadOp);
        attachment.stencilStoreOp = ToVkAttachmentStoreOp(depthStencil.stencilStoreOp);
        attachment.samples = ToVkSampleCountFlagBits(vulkanTexture->getSampleCount());
        attachment.initialLayout = generateDepthStencilInitialLayout(vulkanTexture, depthStencil.depthLoadOp);
        attachment.finalLayout = generateDepthStencilFinalLayout(vulkanTexture);

        vkdescriptor.depthStencilAttachment = attachment;
    }

    {
        VulkanSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // color attachments
        // uint32_t colorAttachmentCount = static_cast<uint32_t>(descriptor.colorAttachments.size());
        uint32_t index = 0;
        for (auto colorAttachment : colorAttachments)
        {
            // attachment references
            VkAttachmentReference colorAttachmentReference{};
            colorAttachmentReference.attachment = index++;
            colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpassDescription.colorAttachments.push_back(colorAttachmentReference);

            if (colorAttachment.resolveView)
            {
                VkAttachmentReference resolveAttachmentReference{};
                resolveAttachmentReference.attachment = index++;
                resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                subpassDescription.resolveAttachments.push_back(resolveAttachmentReference);
            }
        }

        if (depthStencilAttachment.has_value())
        {
            VkAttachmentReference depthAttachment{};
            depthAttachment.attachment = static_cast<uint32_t>(subpassDescription.colorAttachments.size() + subpassDescription.resolveAttachments.size());
            depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            subpassDescription.depthStencilAttachment = depthAttachment;
        }
        vkdescriptor.subpassDescriptions = { subpassDescription };

        VkSubpassDependency subpassDependency{};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (depthStencilAttachment.has_value())
            subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkdescriptor.subpassDependencies = { subpassDependency };
    }

    return vkdescriptor;
}

VulkanFramebufferDescriptor generateVulkanFramebufferDescriptor(std::shared_ptr<VulkanRenderPass> renderPass,
                                                                const std::vector<ColorAttachment>& colorAttachments,
                                                                const std::optional<DepthStencilAttachment>& depthStencilAttachment)
{
    if (colorAttachments.empty())
        throw std::runtime_error("The attachments for color is empty to create frame buffer descriptor.");

    const auto texture = downcast(colorAttachments[0].renderView->getTexture());

    VulkanFramebufferDescriptor vkdescriptor{};
    vkdescriptor.width = texture->getWidth();
    vkdescriptor.height = texture->getHeight();
    vkdescriptor.layers = 1;
    vkdescriptor.renderPass = renderPass->getVkRenderPass();

    for (const auto attachment : colorAttachments)
    {
        FramebufferColorAttachment framebufferColorAttachment{};
        framebufferColorAttachment.renderView = downcast(attachment.renderView)->getVkImageView();

        if (attachment.resolveView)
            framebufferColorAttachment.resolveView = downcast(attachment.resolveView)->getVkImageView();

        vkdescriptor.colorAttachments.push_back(framebufferColorAttachment);
    }

    if (depthStencilAttachment.has_value())
    {
        vkdescriptor.depthStencilAttachment = downcast(depthStencilAttachment.value().textureView)->getVkImageView();
    }

    return vkdescriptor;
}

VulkanRenderPassEncoder::VulkanRenderPassEncoder(VulkanCommandEncoder* commandEncoder, const RenderPassEncoderDescriptor& descriptor)
    : m_commandEncoder(commandEncoder)
    , m_descriptor(descriptor)
{
    BeginRenderPassCommand command{
        { .type = CommandType::kBeginRenderPass },
        .colorAttachments = descriptor.colorAttachments,
        .depthStencilAttachment = descriptor.depthStencilAttachment,
        .occlusionQuerySet = descriptor.occlusionQuerySet,
        .timestampWrites = descriptor.timestampWrites,
    };

    m_commandEncoder->addCommand(std::make_unique<BeginRenderPassCommand>(std::move(command)));

    resetQuery();
}

void VulkanRenderPassEncoder::setPipeline(RenderPipeline* pipeline)
{
    SetRenderPipelineCommand command{ { .type = CommandType::kSetRenderPipeline },
                                      .pipeline = pipeline };

    m_commandEncoder->addCommand(std::make_unique<SetRenderPipelineCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setBindGroup(uint32_t index, BindGroup* bindGroup, std::vector<uint32_t> dynamicOffset)
{
    SetBindGroupCommand command{ { .type = CommandType::kSetRenderBindGroup },
                                 .index = index,
                                 .bindGroup = bindGroup,
                                 .dynamicOffset = dynamicOffset };

    m_commandEncoder->addCommand(std::make_unique<SetBindGroupCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setVertexBuffer(uint32_t slot, Buffer* buffer)
{
    SetVertexBufferCommand command{ { .type = CommandType::kSetVertexBuffer },
                                    .slot = slot,
                                    .buffer = buffer };

    m_commandEncoder->addCommand(std::make_unique<SetVertexBufferCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setIndexBuffer(Buffer* buffer, IndexFormat format)
{
    SetIndexBufferCommand command{ { .type = CommandType::kSetIndexBuffer },
                                   .buffer = buffer,
                                   .format = format };

    m_commandEncoder->addCommand(std::make_unique<SetIndexBufferCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setViewport(float x,
                                          float y,
                                          float width,
                                          float height,
                                          float minDepth,
                                          float maxDepth)
{

    SetViewportCommand command{
        { .type = CommandType::kSetViewport },
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .minDepth = minDepth,
        .maxDepth = maxDepth,
    };

    m_commandEncoder->addCommand(std::make_unique<SetViewportCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setScissor(float x,
                                         float y,
                                         float width,
                                         float height)
{
    SetScissorCommand command{
        { .type = CommandType::kSetScissor },
        .x = x,
        .y = y,
        .width = width,
        .height = height
    };

    m_commandEncoder->addCommand(std::make_unique<SetScissorCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setBlendConstant(const Color& color)
{
    SetBlendConstantCommand command{
        { .type = CommandType::kSetBlendConstant },
        .color = color
    };

    m_commandEncoder->addCommand(std::make_unique<SetBlendConstantCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::draw(uint32_t vertexCount,
                                   uint32_t instanceCount,
                                   uint32_t firstVertex,
                                   uint32_t firstInstance)
{
    DrawCommand command{
        { .type = CommandType::kDraw },
        .vertexCount = vertexCount,
        .instanceCount = instanceCount,
        .firstVertex = firstVertex,
        .firstInstance = firstInstance,
    };

    m_commandEncoder->addCommand(std::make_unique<DrawCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::drawIndexed(uint32_t indexCount,
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
        .firstInstance = firstInstance,
    };

    m_commandEncoder->addCommand(std::make_unique<DrawIndexedCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::executeBundles(const std::vector<RenderBundle*> bundles)
{
    ExecuteBundleCommand command{
        { .type = CommandType::kExecuteBundle },
        .renderBundles = bundles
    };

    m_commandEncoder->addCommand(std::make_unique<ExecuteBundleCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::beginOcclusionQuery(uint32_t queryIndex)
{
    if (m_descriptor.occlusionQuerySet == nullptr)
    {
        throw std::runtime_error("The occlusion query set is nullptr to begin occlusion query.");
    }

    BeginOcclusionQueryCommand command{
        { .type = CommandType::kBeginOcclusionQuery },
        .querySet = m_descriptor.occlusionQuerySet,
        .queryIndex = queryIndex
    };

    m_commandEncoder->addCommand(std::make_unique<BeginOcclusionQueryCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::endOcclusionQuery()
{
    if (m_descriptor.occlusionQuerySet == nullptr)
    {
        throw std::runtime_error("The occlusion query set is nullptr to end occlusion query.");
    }

    EndOcclusionQueryCommand command{
        { .type = CommandType::kEndOcclusionQuery },
        .querySet = m_descriptor.occlusionQuerySet
    };

    m_commandEncoder->addCommand(std::make_unique<EndOcclusionQueryCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::end()
{
    EndRenderPassCommand command{
        { .type = CommandType::kEndRenderPass }
    };

    m_commandEncoder->addCommand(std::make_unique<EndRenderPassCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::nextPass()
{
    // auto vulkanCommandBuffer = downcast(m_commandEncoder)->getCommandBuffer();
    // auto vulkanDevice = downcast(vulkanCommandBuffer->getDevice());

    // vulkanDevice->vkAPI.CmdNextSubpass(vulkanCommandBuffer->getVkCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);
    // ++m_passIndex;
}

void VulkanRenderPassEncoder::resetQuery()
{
    // auto vulkanCommandBuffer = downcast(m_commandEncoder)->getCommandBuffer();
    // auto vulkanDevice = downcast(vulkanCommandBuffer->getDevice());

    // const auto& vkAPI = vulkanDevice->vkAPI;
    // if (m_descriptor.timestampWrites.querySet)
    // {
    //     auto vulkanQuerySet = downcast(m_descriptor.timestampWrites.querySet);
    //     vkAPI.CmdResetQueryPool(vulkanCommandBuffer->getVkCommandBuffer(),
    //                             vulkanQuerySet->getVkQueryPool(),
    //                             0,
    //                             vulkanQuerySet->getCount());
    // }

    // if (m_descriptor.occlusionQuerySet)
    // {
    //     auto vulkanOcclusionQuerySet = downcast(m_descriptor.occlusionQuerySet);
    //     vkAPI.CmdResetQueryPool(vulkanCommandBuffer->getVkCommandBuffer(),
    //                             vulkanOcclusionQuerySet->getVkQueryPool(),
    //                             0,
    //                             vulkanOcclusionQuerySet->getCount());
    // }
}

// Convert Helper
VkIndexType ToVkIndexType(IndexFormat format)
{
    VkIndexType type = VK_INDEX_TYPE_UINT16;
    switch (format)
    {
    case IndexFormat::kUint16:
    default:
        type = VK_INDEX_TYPE_UINT16;
        break;
    case IndexFormat::kUint32:
        type = VK_INDEX_TYPE_UINT32;
        break;
    }

    return type;
}

} // namespace jipu
