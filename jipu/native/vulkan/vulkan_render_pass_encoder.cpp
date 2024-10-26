
#include "vulkan_render_pass_encoder.h"

#include "vulkan_binding_group.h"
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

VkImageLayout generateInitialLayout(const ColorAttachment& colorAttachment)
{
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (colorAttachment.loadOp == LoadOp::kLoad)
    {
        auto renderView = colorAttachment.resolveView ? colorAttachment.resolveView : colorAttachment.renderView;

        layout = downcast(renderView->getTexture())->getFinalLayout();
    }

    return layout;
}

std::vector<VkClearValue> generateClearColor(const RenderPassEncoderDescriptor& descriptor)
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

    addColorClearValue(clearValues, descriptor.colorAttachments);

    if (descriptor.depthStencilAttachment.has_value())
    {
        auto depthStencilAttachment = descriptor.depthStencilAttachment.value();
        if (depthStencilAttachment.depthLoadOp == LoadOp::kClear || depthStencilAttachment.stencilLoadOp == LoadOp::kClear)
        {
            VkClearValue depthStencilClearValue{};
            depthStencilClearValue.depthStencil = { depthStencilAttachment.clearValue.depth,
                                                    depthStencilAttachment.clearValue.stencil };

            clearValues.push_back(depthStencilClearValue);
        }
    }

    return clearValues;
}

} // namespace

VulkanRenderPassDescriptor generateVulkanRenderPassDescriptor(const RenderPassEncoderDescriptor& descriptor)
{
    if (descriptor.colorAttachments.empty())
        throw std::runtime_error("Failed to create vulkan render pass encoder due to empty color attachment.");

    VulkanRenderPassDescriptor vkdescriptor{};

    for (const auto& colorAttachment : descriptor.colorAttachments)
    {
        const auto texture = downcast(colorAttachment.renderView->getTexture());

        VkAttachmentDescription renderAttachment{};
        renderAttachment.format = ToVkFormat(texture->getFormat());
        renderAttachment.loadOp = ToVkAttachmentLoadOp(colorAttachment.loadOp);
        renderAttachment.storeOp = ToVkAttachmentStoreOp(colorAttachment.storeOp);
        renderAttachment.samples = ToVkSampleCountFlagBits(colorAttachment.renderView->getTexture()->getSampleCount());
        renderAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        renderAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        renderAttachment.initialLayout = generateInitialLayout(colorAttachment);
        renderAttachment.finalLayout = texture->getFinalLayout();

        RenderPassColorAttachment renderPassColorAttachment{};
        renderPassColorAttachment.renderAttachment = renderAttachment;

        if (colorAttachment.resolveView)
        {
            const auto texture = downcast(colorAttachment.resolveView->getTexture());

            VkAttachmentDescription resolveAttachment{};
            resolveAttachment.format = ToVkFormat(texture->getFormat());
            resolveAttachment.loadOp = ToVkAttachmentLoadOp(colorAttachment.loadOp);
            resolveAttachment.storeOp = ToVkAttachmentStoreOp(colorAttachment.storeOp);
            resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // should use VK_SAMPLE_COUNT_1_BIT for resolve attachment.
            resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            renderPassColorAttachment.resolveAttachment = resolveAttachment;
        }

        vkdescriptor.colorAttachmentDescriptions.push_back(renderPassColorAttachment);
    }

    if (descriptor.depthStencilAttachment.has_value())
    {
        auto depthStencilAttachment = descriptor.depthStencilAttachment.value();

        const auto texture = downcast(depthStencilAttachment.textureView->getTexture());

        VkAttachmentDescription attachment{};
        attachment.format = ToVkFormat(texture->getFormat());
        attachment.loadOp = ToVkAttachmentLoadOp(depthStencilAttachment.depthLoadOp);
        attachment.storeOp = ToVkAttachmentStoreOp(depthStencilAttachment.depthStoreOp);
        attachment.stencilLoadOp = ToVkAttachmentLoadOp(depthStencilAttachment.stencilLoadOp);
        attachment.stencilStoreOp = ToVkAttachmentStoreOp(depthStencilAttachment.stencilStoreOp);
        attachment.samples = ToVkSampleCountFlagBits(depthStencilAttachment.textureView->getTexture()->getSampleCount());
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        vkdescriptor.depthStencilAttachment = attachment;
    }

    {
        VulkanSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // color attachments
        // uint32_t colorAttachmentCount = static_cast<uint32_t>(descriptor.colorAttachments.size());
        uint32_t index = 0;
        for (auto colorAttachment : descriptor.colorAttachments)
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

        if (descriptor.depthStencilAttachment.has_value())
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
        if (descriptor.depthStencilAttachment.has_value())
            subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkdescriptor.subpassDependencies = { subpassDependency };
    }

    return vkdescriptor;
}

VulkanFramebufferDescriptor generateVulkanFramebufferDescriptor(VulkanRenderPass* renderPass, const RenderPassEncoderDescriptor& descriptor)
{
    if (descriptor.colorAttachments.empty())
        throw std::runtime_error("The attachments for color is empty to create frame buffer descriptor.");

    const auto texture = downcast(descriptor.colorAttachments[0].renderView->getTexture());

    VulkanFramebufferDescriptor vkdescriptor{};
    vkdescriptor.width = texture->getWidth();
    vkdescriptor.height = texture->getHeight();
    vkdescriptor.layers = 1;
    vkdescriptor.renderPass = renderPass;

    for (const auto attachment : descriptor.colorAttachments)
    {
        FramebufferColorAttachment framebufferColorAttachment{
            .renderView = downcast(attachment.renderView),
            .resolveView = downcast(attachment.resolveView),
        };
        vkdescriptor.colorAttachments.push_back(framebufferColorAttachment);
    }

    if (descriptor.depthStencilAttachment.has_value())
    {
        auto depthStencilAttachment = descriptor.depthStencilAttachment.value();
        vkdescriptor.depthStencilAttachment = downcast(depthStencilAttachment.textureView);
    }

    return vkdescriptor;
}

VulkanRenderPassEncoderDescriptor generateVulkanRenderPassEncoderDescriptor(VulkanDevice* device, const RenderPassEncoderDescriptor& descriptor)
{
    auto renderPass = device->getRenderPass(generateVulkanRenderPassDescriptor(descriptor));
    auto framebuffer = device->getFrameBuffer(generateVulkanFramebufferDescriptor(renderPass, descriptor));

    VulkanRenderPassEncoderDescriptor vkdescriptor{};
    vkdescriptor.clearValues = generateClearColor(descriptor);
    vkdescriptor.renderPass = renderPass;
    vkdescriptor.framebuffer = framebuffer;
    vkdescriptor.renderArea.offset = { 0, 0 };
    vkdescriptor.renderArea.extent = { framebuffer->getWidth(), framebuffer->getHeight() };

    // TODO: convert timestampWrites for vulkan.
    vkdescriptor.occlusionQuerySet = descriptor.occlusionQuerySet;
    vkdescriptor.timestampWrites = descriptor.timestampWrites;

    return vkdescriptor;
}

VulkanRenderPassEncoder::VulkanRenderPassEncoder(VulkanCommandEncoder* commandEncoder, const RenderPassEncoderDescriptor& descriptor)
    : VulkanRenderPassEncoder(commandEncoder, generateVulkanRenderPassEncoderDescriptor(commandEncoder->getDevice(), descriptor))
{
}

VulkanRenderPassEncoder::VulkanRenderPassEncoder(VulkanCommandEncoder* commandEncoder, const VulkanRenderPassEncoderDescriptor& descriptor)
    : m_commandEncoder(commandEncoder)
    , m_descriptor(descriptor)
{
    BeginRenderPassCommand command{
        { .type = CommandType::kBeginRenderPass },
        .renderPass = descriptor.renderPass,
        .framebuffer = descriptor.framebuffer,
        .renderArea = descriptor.renderArea,
        .clearValues = descriptor.clearValues,
        .occlusionQuerySet = descriptor.occlusionQuerySet,
        .timestampWrites = descriptor.timestampWrites
    };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.beginRenderPass(&command);
    commandEncodingContext.commands.push_back(std::make_unique<BeginRenderPassCommand>(std::move(command)));

    resetQuery();
}

void VulkanRenderPassEncoder::setPipeline(RenderPipeline* pipeline)
{
    SetRenderPipelineCommand command{ { .type = CommandType::kSetRenderPipeline },
                                      .pipeline = pipeline };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    // commandEncodingContext.resourceTracker.setRenderPipeline(&command);
    commandEncodingContext.commands.push_back(std::make_unique<SetRenderPipelineCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setBindingGroup(uint32_t index, BindingGroup& bindingGroup, std::vector<uint32_t> dynamicOffset)
{
    SetBindGroupCommand command{ { .type = CommandType::kSetRenderBindGroup },
                                 .index = index,
                                 .bindingGroup = &bindingGroup,
                                 .dynamicOffset = dynamicOffset };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.setRenderBindingGroup(&command);
    commandEncodingContext.commands.push_back(std::make_unique<SetBindGroupCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setVertexBuffer(uint32_t slot, Buffer& buffer)
{
    SetVertexBufferCommand command{ { .type = CommandType::kSetVertexBuffer },
                                    .slot = slot,
                                    .buffer = &buffer };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.setVertexBuffer(&command);
    commandEncodingContext.commands.push_back(std::make_unique<SetVertexBufferCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setIndexBuffer(Buffer& buffer, IndexFormat format)
{
    SetIndexBufferCommand command{ { .type = CommandType::kSetIndexBuffer },
                                   .buffer = &buffer,
                                   .format = format };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.setIndexBuffer(&command);
    commandEncodingContext.commands.push_back(std::make_unique<SetIndexBufferCommand>(std::move(command)));
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

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();
    commandEncodingContext.commands.push_back(std::make_unique<SetViewportCommand>(std::move(command)));
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
    auto& commandEncodingContext = downcast(m_commandEncoder)->context();
    commandEncodingContext.commands.push_back(std::make_unique<SetScissorCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::setBlendConstant(const Color& color)
{

    SetBlendConstantCommand command{
        { .type = CommandType::kSetBlendConstant },
        .color = color
    };
    auto& commandEncodingContext = downcast(m_commandEncoder)->context();
    commandEncodingContext.commands.push_back(std::make_unique<SetBlendConstantCommand>(std::move(command)));
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
    auto& commandEncodingContext = downcast(m_commandEncoder)->context();
    commandEncodingContext.commands.push_back(std::make_unique<DrawCommand>(std::move(command)));
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
    auto& commandEncodingContext = downcast(m_commandEncoder)->context();
    commandEncodingContext.commands.push_back(std::make_unique<DrawIndexedCommand>(std::move(command)));
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
    auto& commandEncodingContext = downcast(m_commandEncoder)->context();
    commandEncodingContext.commands.push_back(std::make_unique<BeginOcclusionQueryCommand>(std::move(command)));
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
    auto& commandEncodingContext = downcast(m_commandEncoder)->context();
    commandEncodingContext.commands.push_back(std::make_unique<EndOcclusionQueryCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::end()
{
    EndRenderPassCommand command{
        { .type = CommandType::kEndRenderPass }
    };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.endRenderPass(&command);
    commandEncodingContext.commands.push_back(std::make_unique<EndRenderPassCommand>(std::move(command)));
}

void VulkanRenderPassEncoder::nextPass()
{
    // auto vulkanCommandBuffer = downcast(m_commandEncoder)->getCommandBuffer();
    // auto& vulkanDevice = downcast(vulkanCommandBuffer->getDevice());

    // vulkanDevice.vkAPI.CmdNextSubpass(vulkanCommandBuffer->getVkCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);
    // ++m_passIndex;
}

void VulkanRenderPassEncoder::resetQuery()
{
    // auto vulkanCommandBuffer = downcast(m_commandEncoder)->getCommandBuffer();
    // auto& vulkanDevice = downcast(vulkanCommandBuffer->getDevice());

    // const auto& vkAPI = vulkanDevice.vkAPI;
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
