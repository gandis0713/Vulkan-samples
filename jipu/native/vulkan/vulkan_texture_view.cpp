#include "vulkan_texture_view.h"
#include "jipu/common/hash.h"
#include "vulkan_device.h"
#include "vulkan_texture.h"

#include <stdexcept>

namespace jipu
{

VulkanTextureView::VulkanTextureView(VulkanTexture* texture, const TextureViewDescriptor& descriptor)
    : m_device(downcast(texture->getDevice()))
    , m_texture(texture)
    , m_descriptor(descriptor)
{
    m_imageView = texture->getOrCreateVkImageView(descriptor);
}

VulkanTextureView::~VulkanTextureView()
{
    // do not destroy or release the image view. because the image view is owned by cache in texture.
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
    return m_texture->getWidth();
}

uint32_t VulkanTextureView::getHeight() const
{
    return m_texture->getHeight();
}

uint32_t VulkanTextureView::getDepth() const
{
    return m_texture->getDepth();
}

uint32_t VulkanTextureView::getBaseMipLevel() const
{
    return m_descriptor.baseMipLevel;
}

uint32_t VulkanTextureView::getMipLevelCount() const
{
    return m_descriptor.mipLevelCount;
}

uint32_t VulkanTextureView::getBaseArrayLayer() const
{
    return m_descriptor.baseArrayLayer;
}

uint32_t VulkanTextureView::getArrayLayerCount() const
{
    return m_descriptor.arrayLayerCount;
}

Texture* VulkanTextureView::getTexture() const
{
    return m_texture;
}

VkImageView VulkanTextureView::getVkImageView() const
{
    return m_imageView;
}

// VulkanImageViewCache

size_t VulkanImageViewCache::Functor::operator()(const TextureViewDescriptor& descriptor) const
{
    size_t hash = 0;

    combineHash(hash, descriptor.dimension);
    combineHash(hash, descriptor.aspect);
    combineHash(hash, descriptor.baseMipLevel);
    combineHash(hash, descriptor.mipLevelCount);
    combineHash(hash, descriptor.baseArrayLayer);
    combineHash(hash, descriptor.arrayLayerCount);

    return hash;
}

bool VulkanImageViewCache::Functor::operator()(const TextureViewDescriptor& lhs,
                                               const TextureViewDescriptor& rhs) const
{
    if (lhs.dimension != rhs.dimension ||
        lhs.aspect != rhs.aspect ||
        lhs.baseMipLevel != rhs.baseMipLevel ||
        lhs.mipLevelCount != rhs.mipLevelCount ||
        lhs.baseArrayLayer != rhs.baseArrayLayer ||
        lhs.arrayLayerCount != rhs.arrayLayerCount)
    {
        return false;
    }

    return true;
}

VulkanImageViewCache::VulkanImageViewCache(VulkanTexture* texture)
    : m_texture(texture)
{
}

VkImageView VulkanImageViewCache::getVkImageView(const TextureViewDescriptor& descriptor)
{
    auto it = m_cache.find(descriptor);
    if (it != m_cache.end())
    {
        return it->second;
    }

    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = m_texture->getVkImage();
    imageViewCreateInfo.viewType = ToVkImageViewType(descriptor.dimension);
    imageViewCreateInfo.format = ToVkFormat(m_texture->getFormat());

    imageViewCreateInfo.components = VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                                         VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

    imageViewCreateInfo.subresourceRange.aspectMask = ToVkImageAspectFlags(descriptor.aspect);
    imageViewCreateInfo.subresourceRange.baseMipLevel = descriptor.baseMipLevel;
    imageViewCreateInfo.subresourceRange.levelCount = descriptor.mipLevelCount;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = descriptor.baseArrayLayer;
    imageViewCreateInfo.subresourceRange.layerCount = descriptor.arrayLayerCount;

    auto vulkanDevice = downcast(m_texture->getDevice());
    VkImageView imageView = VK_NULL_HANDLE;
    if (vulkanDevice->vkAPI.CreateImageView(vulkanDevice->getVkDevice(), &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image views!");
    }

    m_cache[descriptor] = imageView;

    return imageView;
}

void VulkanImageViewCache::clear()
{
    for (auto& [_, imageView] : m_cache)
    {
        m_texture->getDevice()->getDeleter()->safeDestroy(imageView);
    }
    m_cache.clear();
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
