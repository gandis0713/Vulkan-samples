#include "vulkan_texture_view.h"
#include "vulkan_device.h"
#include "vulkan_texture.h"

#include <stdexcept>

namespace jipu
{

VulkanTextureView::VulkanTextureView(VulkanTexture& texture, const TextureViewDescriptor& descriptor)
    : m_texture(texture)
    , m_descriptor(descriptor)
{
    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = texture.getVkImage();
    imageViewCreateInfo.viewType = ToVkImageViewType(descriptor.dimension);
    imageViewCreateInfo.format = ToVkFormat(texture.getFormat());

    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    imageViewCreateInfo.subresourceRange.aspectMask = ToVkImageAspectFlags(descriptor.aspect);
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = texture.getMipLevels();
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    auto& vulkanDevice = downcast(m_texture.getDevice());
    if (vulkanDevice.vkAPI.CreateImageView(vulkanDevice.getVkDevice(), &imageViewCreateInfo, nullptr, &m_imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image views!");
    }
}

VulkanTextureView::~VulkanTextureView()
{
    auto& vulkanDevice = downcast(m_texture.getDevice());

    auto framebufferCache = vulkanDevice.getFramebufferCache();
    framebufferCache->invalidate(this);

    vulkanDevice.vkAPI.DestroyImageView(vulkanDevice.getVkDevice(), m_imageView, nullptr);
}

TextureViewDimension VulkanTextureView::getDimension() const
{
    return m_descriptor.dimension;
}

TextureAspectFlags VulkanTextureView::getAspect() const
{
    return m_descriptor.aspect;
}

uint32_t VulkanTextureView::getWidth() const
{
    return m_texture.getWidth();
}

uint32_t VulkanTextureView::getHeight() const
{
    return m_texture.getHeight();
}

uint32_t VulkanTextureView::getDepth() const
{
    return m_texture.getDepth();
}

Texture* VulkanTextureView::getTexture() const
{
    return &m_texture;
}

VkImageView VulkanTextureView::getVkImageView() const
{
    return m_imageView;
}

// Convert Helper

VkImageViewType ToVkImageViewType(TextureViewDimension type)
{
    switch (type)
    {
    case TextureViewDimension::k1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case TextureViewDimension::k2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureViewDimension::k2DArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case TextureViewDimension::k3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureViewDimension::kCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case TextureViewDimension::kCubeArray:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    default:
        throw std::runtime_error(fmt::format("{} type does not support.", static_cast<uint32_t>(type)));
    }
}
TextureViewDimension ToTextureViewDimension(VkImageViewType type)
{
    switch (type)
    {
    case VK_IMAGE_VIEW_TYPE_1D:
        return TextureViewDimension::k1D;
    case VK_IMAGE_VIEW_TYPE_2D:
        return TextureViewDimension::k2D;
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
        return TextureViewDimension::k2DArray;
    case VK_IMAGE_VIEW_TYPE_3D:
        return TextureViewDimension::k3D;
    case VK_IMAGE_VIEW_TYPE_CUBE:
        return TextureViewDimension::kCube;
    case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
        return TextureViewDimension::kCubeArray;
    default:
        throw std::runtime_error(fmt::format("{} type does not support.", static_cast<uint32_t>(type)));
    }
}

VkImageAspectFlags ToVkImageAspectFlags(TextureAspectFlags flags)
{
    VkImageAspectFlags vkflags = 0u;

    if (flags & TextureAspectFlagBits::kColor)
    {
        vkflags |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if (flags & TextureAspectFlagBits::kDepth)
    {
        vkflags |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (flags & TextureAspectFlagBits::kStencil)
    {
        vkflags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    return vkflags;
}

TextureAspectFlags ToTextureAspectFlags(VkImageAspectFlags vkflags)
{
    TextureAspectFlags flags = TextureAspectFlagBits::kUndefined;

    if (vkflags & VK_IMAGE_ASPECT_COLOR_BIT)
    {
        flags |= TextureAspectFlagBits::kColor;
    }
    if (vkflags & VK_IMAGE_ASPECT_DEPTH_BIT)
    {
        flags |= TextureAspectFlagBits::kDepth;
    }
    if (vkflags & VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        flags |= TextureAspectFlagBits::kStencil;
    }

    return flags;
}

} // namespace jipu
