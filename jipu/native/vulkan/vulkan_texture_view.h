#pragma once

#include "jipu/common/cast.h"
#include "texture_view.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

class VulkanDevice;
class VulkanTexture;
class VULKAN_EXPORT VulkanTextureView : public TextureView
{
public:
    VulkanTextureView() = delete;
    VulkanTextureView(VulkanTexture* texture, const TextureViewDescriptor& descriptor);
    ~VulkanTextureView() override;

public:
    Texture* getTexture() const override;
    TextureViewDimension getDimension() const override;
    TextureAspectFlags getAspect() const override;
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    uint32_t getDepth() const override;
    uint32_t getBaseMipLevel() const override;
    uint32_t getMipLevelCount() const override;
    uint32_t getBaseArrayLayer() const override;
    uint32_t getArrayLayerCount() const override;

public:
    VkImageView getVkImageView() const;

protected:
    VulkanDevice* m_device = nullptr;
    VulkanTexture* m_texture = nullptr;
    const TextureViewDescriptor m_descriptor{};

protected:
    VkImageView m_imageView = VK_NULL_HANDLE;
};
DOWN_CAST(VulkanTextureView, TextureView);

class VULKAN_EXPORT VulkanImageViewCache final
{
public:
    VulkanImageViewCache(VulkanTexture* texture);
    ~VulkanImageViewCache() = default;

    VkImageView getVkImageView(const TextureViewDescriptor& descriptor);

    void clear();

private:
    VulkanTexture* m_texture = nullptr;

private:
    struct Functor
    {
        size_t operator()(const TextureViewDescriptor& descriptor) const;
        bool operator()(const TextureViewDescriptor& lhs, const TextureViewDescriptor& rhs) const;
    };
    using Cache = std::unordered_map<TextureViewDescriptor, VkImageView, Functor, Functor>;

    Cache m_cache{};
};

// Convert Helper
VkImageViewType ToVkImageViewType(TextureViewDimension type);
TextureViewDimension ToTextureViewDimension(VkImageViewType type);
VkImageAspectFlags ToVkImageAspectFlags(TextureAspectFlags flags);
TextureAspectFlags ToTextureAspectFlags(VkImageAspectFlags flags);

} // namespace jipu
