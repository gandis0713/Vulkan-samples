#pragma once

#include "render_pass_encoder.h"
#include "texture.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace jipu
{

struct VulkanSubpassDescription
{
    VkSubpassDescriptionFlags flags;
    VkPipelineBindPoint pipelineBindPoint;
    std::vector<VkAttachmentReference> inputAttachments{};
    std::vector<VkAttachmentReference> colorAttachments{};
    std::vector<VkAttachmentReference> resolveAttachments{};
    std::optional<VkAttachmentReference> depthStencilAttachment = std::nullopt;
    std::vector<uint32_t> preserveAttachments{};
};

struct RenderPassColorAttachment
{
    VkAttachmentDescription renderAttachment{};
    std::optional<VkAttachmentDescription> resolveAttachment = std::nullopt;
};

struct VulkanRenderPassDescriptor
{
    const void* next;
    VkRenderPassCreateFlags flags;
    std::vector<RenderPassColorAttachment> colorAttachmentDescriptions{};
    std::optional<VkAttachmentDescription> depthStencilAttachment = std::nullopt;
    std::vector<VulkanSubpassDescription> subpassDescriptions{};
    std::vector<VkSubpassDependency> subpassDependencies{};
};

class VulkanDevice;
class VULKAN_EXPORT VulkanRenderPass
{
public:
    VulkanRenderPass() = delete;
    VulkanRenderPass(VulkanDevice* device, const VulkanRenderPassDescriptor& descriptor);
    ~VulkanRenderPass();

public:
    const std::vector<RenderPassColorAttachment>& getColorAttachments() const;

public:
    VkRenderPass getVkRenderPass() const;

private:
    VulkanDevice* m_device = nullptr;
    VulkanRenderPassDescriptor m_descriptor{};

private:
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
};

class VULKAN_EXPORT VulkanRenderPassCache final
{

public:
    VulkanRenderPassCache(VulkanDevice* device);
    ~VulkanRenderPassCache() = default;

    std::shared_ptr<VulkanRenderPass> getRenderPass(const VulkanRenderPassDescriptor& descriptor);

    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    struct Functor
    {
        size_t operator()(const VulkanRenderPassDescriptor& descriptor) const;
        bool operator()(const VulkanRenderPassDescriptor& lhs, const VulkanRenderPassDescriptor& rhs) const;
    };
    using Cache = std::unordered_map<VulkanRenderPassDescriptor, std::shared_ptr<VulkanRenderPass>, Functor, Functor>;

    Cache m_cache{};
};

// Convert Helper
VkAttachmentLoadOp ToVkAttachmentLoadOp(LoadOp loadOp);
LoadOp ToVkAttachmentLoadOp(VkAttachmentLoadOp loadOp);
VkAttachmentStoreOp ToVkAttachmentStoreOp(StoreOp storeOp);
StoreOp ToStoreOp(VkAttachmentStoreOp storeOp);

} // namespace jipu
