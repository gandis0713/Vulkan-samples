#include "vulkan_framebuffer.h"

#include "vulkan_device.h"
#include "vulkan_render_pass.h"
#include "vulkan_texture_view.h"

#include "jipu/common/hash.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice* device, const VulkanFramebufferDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    std::vector<VkImageView> attachments{};
    for (const auto attachment : descriptor.colorAttachments)
    {
        attachments.push_back(attachment.renderView);
        if (attachment.resolveView)
            attachments.push_back(attachment.resolveView);
    }

    if (descriptor.depthStencilAttachment)
    {
        attachments.push_back(descriptor.depthStencilAttachment);
    }

    VkFramebufferCreateInfo framebufferCreateInfo{ .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                   .pNext = descriptor.next,
                                                   .flags = descriptor.flags,
                                                   .renderPass = descriptor.renderPass,
                                                   .attachmentCount = static_cast<uint32_t>(attachments.size()),
                                                   .pAttachments = attachments.data(),
                                                   .width = descriptor.width,
                                                   .height = descriptor.height,
                                                   .layers = descriptor.layers };

    if (m_device->vkAPI.CreateFramebuffer(device->getVkDevice(), &framebufferCreateInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

VulkanFramebuffer::~VulkanFramebuffer()
{
    m_device->getDeleter()->safeDestroy(m_framebuffer);
}

const std::vector<FramebufferColorAttachment>& VulkanFramebuffer::getColorAttachments() const
{
    return m_descriptor.colorAttachments;
}

VkImageView VulkanFramebuffer::getDepthStencilAttachment() const
{
    return m_descriptor.depthStencilAttachment;
}

uint32_t VulkanFramebuffer::getWidth() const
{
    return m_descriptor.width;
}

uint32_t VulkanFramebuffer::getHeight() const
{
    return m_descriptor.height;
}

VkFramebuffer VulkanFramebuffer::getVkFrameBuffer() const
{
    return m_framebuffer;
}

size_t VulkanFramebufferCache::Functor::operator()(const VulkanFramebufferDescriptor& descriptor) const
{
    size_t hash = 0;

    combineHash(hash, descriptor.renderPass);
    combineHash(hash, descriptor.flags);
    combineHash(hash, descriptor.width);
    combineHash(hash, descriptor.height);
    combineHash(hash, descriptor.layers);

    for (const auto& attachment : descriptor.colorAttachments)
    {
        combineHash(hash, reinterpret_cast<uint64_t>(attachment.renderView));
        if (attachment.resolveView)
            combineHash(hash, reinterpret_cast<uint64_t>(attachment.resolveView));
    }

    if (descriptor.depthStencilAttachment)
    {
        combineHash(hash, reinterpret_cast<uint64_t>(descriptor.depthStencilAttachment));
    }

    return hash;
}

bool VulkanFramebufferCache::Functor::operator()(const VulkanFramebufferDescriptor& lhs,
                                                 const VulkanFramebufferDescriptor& rhs) const
{
    if (lhs.flags == rhs.flags &&
        lhs.width == rhs.width &&
        lhs.height == rhs.height &&
        lhs.layers == rhs.layers &&
        lhs.renderPass == rhs.renderPass &&
        lhs.colorAttachments.size() == rhs.colorAttachments.size())
    {
        for (auto i = 0; i < lhs.colorAttachments.size(); ++i)
        {
            if (lhs.colorAttachments[i].renderView != rhs.colorAttachments[i].renderView)
            {
                return false;
            }

            if (lhs.colorAttachments[i].resolveView && rhs.colorAttachments[i].resolveView)
            {
                if (lhs.colorAttachments[i].resolveView != rhs.colorAttachments[i].resolveView)
                {
                    return false;
                }
            }
        }

        if (lhs.depthStencilAttachment && rhs.depthStencilAttachment)
        {
            if (lhs.depthStencilAttachment != rhs.depthStencilAttachment)
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

VulkanFramebufferCache::VulkanFramebufferCache(VulkanDevice* device)
    : m_device(device)
{
    // TODO
}

std::shared_ptr<VulkanFramebuffer> VulkanFramebufferCache::getFrameBuffer(const VulkanFramebufferDescriptor& descriptor)
{
    auto it = m_cache.find(descriptor);
    if (it != m_cache.end())
    {
        return it->second;
    }
    auto framebuffer = std::make_shared<VulkanFramebuffer>(m_device, descriptor);

    m_cache.emplace(descriptor, framebuffer);

    return framebuffer;
}

bool VulkanFramebufferCache::invalidate(VkImageView imageView)
{
    bool isInvalidated = false;

    for (auto it = m_cache.begin(); it != m_cache.end(); /* no increment */)
    {
        auto& descriptor = it->first;
        bool shouldErase = false;

        for (auto& attachment : descriptor.colorAttachments)
        {
            if ((attachment.renderView && attachment.renderView == imageView) ||
                (attachment.resolveView && attachment.resolveView == imageView))
            {
                shouldErase = true;
                break;
            }
        }

        if (!shouldErase && descriptor.depthStencilAttachment &&
            descriptor.depthStencilAttachment == imageView)
        {
            shouldErase = true;
        }

        if (shouldErase)
        {
            it = m_cache.erase(it);
            isInvalidated = true;
        }
        else
        {
            ++it;
        }
    }

    return isInvalidated;
}

bool VulkanFramebufferCache::invalidate(VkRenderPass renderPass)
{
    bool invalidated = false;

    for (auto it = m_cache.begin(); it != m_cache.end(); /* no increment */)
    {
        auto& descriptor = it->first;
        if (descriptor.renderPass == renderPass)
        {
            it = m_cache.erase(it);
            invalidated = true;
        }
        else
        {
            ++it;
        }
    }

    return invalidated;
}

void VulkanFramebufferCache::clear()
{
    m_cache.clear();
}

} // namespace jipu
