#pragma once

#include "jipu/common/cast.h"
#include "texture_view.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

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

public:
    VkImageView getVkImageView() const;

protected:
    VulkanTexture* m_texture = nullptr;
    const TextureViewDescriptor m_descriptor{};

protected:
    VkImageView m_imageView = VK_NULL_HANDLE;
};

DOWN_CAST(VulkanTextureView, TextureView);

// Convert Helper
VkImageViewType ToVkImageViewType(TextureViewDimension type);
TextureViewDimension ToTextureViewDimension(VkImageViewType type);
VkImageAspectFlags ToVkImageAspectFlags(TextureAspectFlags flags);
TextureAspectFlags ToTextureAspectFlags(VkImageAspectFlags flags);

} // namespace jipu
