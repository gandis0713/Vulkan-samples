#include "vulkan_render_pass.h"
#include "jipu/common/hash.h"
#include "vulkan_device.h"
#include "vulkan_texture.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, const VulkanRenderPassDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    std::vector<VkAttachmentDescription> attachmentDescriptions{};
    for (const auto& colorAttachment : descriptor.colorAttachmentDescriptions)
    {
        attachmentDescriptions.push_back(colorAttachment.renderAttachment);
        if (colorAttachment.resolveAttachment.has_value())
            attachmentDescriptions.push_back(colorAttachment.resolveAttachment.value());
    }

    if (descriptor.depthStencilAttachment.has_value())
        attachmentDescriptions.push_back(descriptor.depthStencilAttachment.value());

    std::vector<VkSubpassDescription> subpassDescriptions{};
    subpassDescriptions.resize(descriptor.subpassDescriptions.size());
    for (auto i = 0; i < descriptor.subpassDescriptions.size(); ++i)
    {
        const auto& subpassDescription = descriptor.subpassDescriptions[i];

        subpassDescriptions[i].flags = subpassDescription.flags;
        subpassDescriptions[i].pipelineBindPoint = subpassDescription.pipelineBindPoint;
        subpassDescriptions[i].colorAttachmentCount = static_cast<uint32_t>(subpassDescription.colorAttachments.size());
        subpassDescriptions[i].pColorAttachments = subpassDescription.colorAttachments.data();
        subpassDescriptions[i].inputAttachmentCount = static_cast<uint32_t>(subpassDescription.inputAttachments.size());
        subpassDescriptions[i].pInputAttachments = subpassDescription.inputAttachments.data();
        subpassDescriptions[i].pResolveAttachments = subpassDescription.resolveAttachments.data();
        subpassDescriptions[i].preserveAttachmentCount = static_cast<uint32_t>(subpassDescription.preserveAttachments.size());
        subpassDescriptions[i].pPreserveAttachments = subpassDescription.preserveAttachments.data();
        if (subpassDescription.depthStencilAttachment.has_value())
            subpassDescriptions[i].pDepthStencilAttachment = &subpassDescription.depthStencilAttachment.value();
    }

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
    renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
    renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(descriptor.subpassDependencies.size());
    renderPassCreateInfo.pDependencies = descriptor.subpassDependencies.data();

    if (device->vkAPI.CreateRenderPass(device->getVkDevice(), &renderPassCreateInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

VulkanRenderPass::~VulkanRenderPass()
{
    m_device->getDeleter()->safeDestroy(m_renderPass);
}

const std::vector<RenderPassColorAttachment>& VulkanRenderPass::getColorAttachments() const
{
    return m_descriptor.colorAttachmentDescriptions;
}

VkRenderPass VulkanRenderPass::getVkRenderPass() const
{
    return m_renderPass;
}

size_t VulkanRenderPassCache::Functor::operator()(const VulkanRenderPassDescriptor& descriptor) const
{
    size_t hash = 0;

    combineHash(hash, descriptor.flags);

    for (const auto& colorAttachment : descriptor.colorAttachmentDescriptions)
    {
        combineHash(hash, colorAttachment.renderAttachment.finalLayout);
        combineHash(hash, colorAttachment.renderAttachment.flags);
        combineHash(hash, colorAttachment.renderAttachment.format);
        combineHash(hash, colorAttachment.renderAttachment.initialLayout);
        combineHash(hash, colorAttachment.renderAttachment.loadOp);
        combineHash(hash, colorAttachment.renderAttachment.samples);
        combineHash(hash, colorAttachment.renderAttachment.stencilLoadOp);
        combineHash(hash, colorAttachment.renderAttachment.stencilStoreOp);
        combineHash(hash, colorAttachment.renderAttachment.storeOp);

        if (colorAttachment.resolveAttachment.has_value())
        {
            const auto& resolveAttachment = colorAttachment.resolveAttachment.value();
            combineHash(hash, resolveAttachment.finalLayout);
            combineHash(hash, resolveAttachment.flags);
            combineHash(hash, resolveAttachment.format);
            combineHash(hash, resolveAttachment.initialLayout);
            combineHash(hash, resolveAttachment.loadOp);
            combineHash(hash, resolveAttachment.samples);
            combineHash(hash, resolveAttachment.stencilLoadOp);
            combineHash(hash, resolveAttachment.stencilStoreOp);
            combineHash(hash, resolveAttachment.storeOp);
        }
    }

    for (auto subpass : descriptor.subpassDescriptions)
    {
        combineHash(hash, subpass.flags);
        combineHash(hash, subpass.colorAttachments.size());
        combineHash(hash, subpass.inputAttachments.size());
        combineHash(hash, subpass.resolveAttachments.size());
        combineHash(hash, subpass.preserveAttachments.size());
        combineHash(hash, subpass.depthStencilAttachment.has_value());
    }

    for (auto subpass : descriptor.subpassDependencies)
    {
        combineHash(hash, subpass.dependencyFlags);
        combineHash(hash, subpass.dstAccessMask);
        combineHash(hash, subpass.dstStageMask);
        combineHash(hash, subpass.dstSubpass);
        combineHash(hash, subpass.srcAccessMask);
        combineHash(hash, subpass.srcStageMask);
        combineHash(hash, subpass.srcSubpass);
    }

    return hash;
}

bool VulkanRenderPassCache::Functor::operator()(const VulkanRenderPassDescriptor& lhs,
                                                const VulkanRenderPassDescriptor& rhs) const
{
    if (lhs.colorAttachmentDescriptions.size() != rhs.colorAttachmentDescriptions.size() ||
        lhs.depthStencilAttachment.has_value() != rhs.depthStencilAttachment.has_value() ||
        lhs.subpassDescriptions.size() != rhs.subpassDescriptions.size() ||
        lhs.subpassDependencies.size() != rhs.subpassDependencies.size())
    {
        return false;
    }

    for (auto i = 0; i < lhs.colorAttachmentDescriptions.size(); ++i)
    {
        if (lhs.colorAttachmentDescriptions[i].renderAttachment.finalLayout != rhs.colorAttachmentDescriptions[i].renderAttachment.finalLayout ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.flags != rhs.colorAttachmentDescriptions[i].renderAttachment.flags ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.format != rhs.colorAttachmentDescriptions[i].renderAttachment.format ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.initialLayout != rhs.colorAttachmentDescriptions[i].renderAttachment.initialLayout ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.loadOp != rhs.colorAttachmentDescriptions[i].renderAttachment.loadOp ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.samples != rhs.colorAttachmentDescriptions[i].renderAttachment.samples ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.stencilLoadOp != rhs.colorAttachmentDescriptions[i].renderAttachment.stencilLoadOp ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.stencilStoreOp != rhs.colorAttachmentDescriptions[i].renderAttachment.stencilStoreOp ||
            lhs.colorAttachmentDescriptions[i].renderAttachment.storeOp != rhs.colorAttachmentDescriptions[i].renderAttachment.storeOp)
        {
            return false;
        }

        if (lhs.colorAttachmentDescriptions[i].resolveAttachment.has_value() && rhs.colorAttachmentDescriptions[i].resolveAttachment.has_value())
        {
            auto lhsAttachmentDescription = lhs.colorAttachmentDescriptions[i].resolveAttachment.value();
            auto rhsAttachmentDescription = rhs.colorAttachmentDescriptions[i].resolveAttachment.value();

            if (lhsAttachmentDescription.finalLayout != rhsAttachmentDescription.finalLayout ||
                lhsAttachmentDescription.flags != rhsAttachmentDescription.flags ||
                lhsAttachmentDescription.format != rhsAttachmentDescription.format ||
                lhsAttachmentDescription.initialLayout != rhsAttachmentDescription.initialLayout ||
                lhsAttachmentDescription.loadOp != rhsAttachmentDescription.loadOp ||
                lhsAttachmentDescription.samples != rhsAttachmentDescription.samples ||
                lhsAttachmentDescription.stencilLoadOp != rhsAttachmentDescription.stencilLoadOp ||
                lhsAttachmentDescription.stencilStoreOp != rhsAttachmentDescription.stencilStoreOp ||
                lhsAttachmentDescription.storeOp != rhsAttachmentDescription.storeOp)
            {
                return false;
            }
        }
    }

    for (auto i = 0; i < lhs.subpassDescriptions.size(); ++i)
    {
        if (lhs.subpassDescriptions[i].flags != rhs.subpassDescriptions[i].flags ||
            lhs.subpassDescriptions[i].colorAttachments.size() != rhs.subpassDescriptions[i].colorAttachments.size() ||
            lhs.subpassDescriptions[i].inputAttachments.size() != rhs.subpassDescriptions[i].inputAttachments.size() ||
            lhs.subpassDescriptions[i].resolveAttachments.size() != rhs.subpassDescriptions[i].resolveAttachments.size() ||
            lhs.subpassDescriptions[i].preserveAttachments.size() != rhs.subpassDescriptions[i].preserveAttachments.size() ||
            lhs.subpassDescriptions[i].depthStencilAttachment.has_value() != rhs.subpassDescriptions[i].depthStencilAttachment.has_value())
        {
            return false;
        }
    }

    for (auto i = 0; i < lhs.subpassDependencies.size(); ++i)
    {
        if (lhs.subpassDependencies[i].dependencyFlags != rhs.subpassDependencies[i].dependencyFlags ||
            lhs.subpassDependencies[i].dstAccessMask != rhs.subpassDependencies[i].dstAccessMask ||
            lhs.subpassDependencies[i].dstStageMask != rhs.subpassDependencies[i].dstStageMask ||
            lhs.subpassDependencies[i].dstSubpass != rhs.subpassDependencies[i].dstSubpass ||
            lhs.subpassDependencies[i].srcAccessMask != rhs.subpassDependencies[i].srcAccessMask ||
            lhs.subpassDependencies[i].srcStageMask != rhs.subpassDependencies[i].srcStageMask ||
            lhs.subpassDependencies[i].srcSubpass != rhs.subpassDependencies[i].srcSubpass)
        {
            return false;
        }
    }

    return true;
}

VulkanRenderPassCache::VulkanRenderPassCache(VulkanDevice* device)
    : m_device(device)
{
}

std::shared_ptr<VulkanRenderPass> VulkanRenderPassCache::getRenderPass(const VulkanRenderPassDescriptor& descriptor)
{
    auto it = m_cache.find(descriptor);
    if (it != m_cache.end())
    {
        return it->second;
    }

    // create new renderpass
    std::shared_ptr<VulkanRenderPass> renderPass = std::make_shared<VulkanRenderPass>(m_device, descriptor);

    auto result = m_cache.emplace(descriptor, renderPass);

    return renderPass;
}

void VulkanRenderPassCache::clear()
{
    m_cache.clear();
}

// Convert Helper
VkAttachmentLoadOp ToVkAttachmentLoadOp(LoadOp loadOp)
{
    switch (loadOp)
    {
    case LoadOp::kClear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;

    case LoadOp::kLoad:
        return VK_ATTACHMENT_LOAD_OP_LOAD;

    case LoadOp::kDontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

    default:
        spdlog::error("{} Load Op type is not supported.", static_cast<uint8_t>(loadOp));
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

LoadOp ToVkAttachmentLoadOp(VkAttachmentLoadOp loadOp)
{
    switch (loadOp)
    {
    case VK_ATTACHMENT_LOAD_OP_CLEAR:
        return LoadOp::kClear;

    case VK_ATTACHMENT_LOAD_OP_LOAD:
        return LoadOp::kLoad;

    case VK_ATTACHMENT_LOAD_OP_DONT_CARE:
        return LoadOp::kDontCare;

    default:
        spdlog::error("{} Load Op type is not supported.", static_cast<int32_t>(loadOp));
        return LoadOp::kDontCare;
    }
}

VkAttachmentStoreOp ToVkAttachmentStoreOp(StoreOp storeOp)
{
    switch (storeOp)
    {
    case StoreOp::kStore:
        return VK_ATTACHMENT_STORE_OP_STORE;

    case StoreOp::kDontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;

    default:
        spdlog::error("{} Store Op type is not supported.", static_cast<uint8_t>(storeOp));
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

StoreOp ToStoreOp(VkAttachmentStoreOp storeOp)
{
    switch (storeOp)
    {
    case VK_ATTACHMENT_STORE_OP_STORE:
        return StoreOp::kStore;

    case VK_ATTACHMENT_STORE_OP_DONT_CARE:
        return StoreOp::kDontCare;

    default:
        spdlog::error("{} Store Op type is not supported.", static_cast<uint8_t>(storeOp));
        return StoreOp::kDontCare;
    }
}

} // namespace jipu
