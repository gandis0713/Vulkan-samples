#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

class VulkanRenderPass;
class VulkanTextureView;
struct FramebufferColorAttachment
{
    VkImageView renderView = VK_NULL_HANDLE;
    VkImageView resolveView = VK_NULL_HANDLE;
};

struct VulkanFramebufferDescriptor
{
    const void* next = nullptr;
    VkFramebufferCreateFlags flags = 0u;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<FramebufferColorAttachment> colorAttachments{};
    VkImageView depthStencilAttachment = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 0;
};

class VulkanDevice;
class VULKAN_EXPORT VulkanFramebuffer
{
public:
    VulkanFramebuffer() = delete;
    VulkanFramebuffer(VulkanDevice* device, const VulkanFramebufferDescriptor& descriptor);
    ~VulkanFramebuffer();

public:
    const std::vector<FramebufferColorAttachment>& getColorAttachments() const;
    VkImageView getDepthStencilAttachment() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;

public:
    VkFramebuffer getVkFrameBuffer() const;

private:
    VulkanDevice* m_device = nullptr;
    const VulkanFramebufferDescriptor m_descriptor{};

private:
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
};

class VULKAN_EXPORT VulkanFramebufferCache final
{

public:
    VulkanFramebufferCache(VulkanDevice* device);
    ~VulkanFramebufferCache() = default;

    std::shared_ptr<VulkanFramebuffer> getFrameBuffer(const VulkanFramebufferDescriptor& descriptor);

    bool invalidate(VkImageView imageView);
    bool invalidate(VkRenderPass renderPass);

    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    struct Functor
    {
        // hash key
        size_t operator()(const VulkanFramebufferDescriptor& descriptor) const;
        // equal
        bool operator()(const VulkanFramebufferDescriptor& lhs, const VulkanFramebufferDescriptor& rhs) const;
    };
    using Cache = std::unordered_map<VulkanFramebufferDescriptor, std::shared_ptr<VulkanFramebuffer>, Functor, Functor>;

    Cache m_cache{};
};

} // namespace jipu
