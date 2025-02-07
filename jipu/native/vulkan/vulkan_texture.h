#pragma once

#include "jipu/common/assert.h"
#include "jipu/common/cast.h"
#include "texture.h"
#include "vulkan_api.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_export.h"
#include "vulkan_resource.h"
#include "vulkan_texture_view.h"

#include <fmt/format.h>
#include <vector>

namespace jipu
{

enum class VulkanTextureOwner
{
    kSelf,
    kSwapchain
};

struct VulkanTextureDescriptor
{
    const void* next = nullptr;
    VkImageCreateFlags flags;
    VkImageType imageType;
    VkFormat format;
    VkExtent3D extent;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkSharingMode sharingMode;
    std::vector<uint32_t> queueFamilyIndices{};
    VkImageLayout initialLayout;

    // if image created by other.
    VulkanTextureOwner owner = VulkanTextureOwner::kSelf;
    VkImage image = VK_NULL_HANDLE;
};

class VulkanDevice;
class VULKAN_EXPORT VulkanTexture : public Texture
{
public:
    VulkanTexture() = delete;
    VulkanTexture(VulkanDevice* device, const TextureDescriptor& descriptor);
    VulkanTexture(VulkanDevice* device, const VulkanTextureDescriptor& descriptor);
    ~VulkanTexture() override;

    std::unique_ptr<TextureView> createTextureView(const TextureViewDescriptor& descriptor) override;

public:
    TextureType getType() const override;
    TextureFormat getFormat() const override;
    TextureUsageFlags getUsage() const override;
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    uint32_t getDepth() const override;
    uint32_t getMipLevels() const override;
    uint32_t getSampleCount() const override;

public:
    VulkanDevice* getDevice() const;

public:
    VkImage getVkImage() const;
    VulkanMemory getVulkanMemory() const;
    VulkanTextureResource getVulkanTextureResource() const;

    VkImageLayout getCurrentLayout(uint32_t mipLevel = 0) const;
    /// @brief generate final layout by usage.
    /// @return VKImageLayout
    VkImageLayout getFinalLayout() const;

    /// @brief record pipeline barrier command, but not submitted.
    void cmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImageMemoryBarrier barrier);

    VulkanTextureOwner getOwner() const;
    bool isDepthStencil() const;

    VkImageView getOrCreateVkImageView(const TextureViewDescriptor& descriptor);

protected:
    VulkanDevice* m_device = nullptr;
    const VulkanTextureDescriptor m_descriptor{};
    std::vector<VkImageLayout> m_layouts{};

private:
    VulkanTextureResource m_resource;
    VulkanTextureOwner m_owner;

    std::unique_ptr<VulkanImageViewCache> m_imageViewCache = nullptr;
};

DOWN_CAST(VulkanTexture, Texture);

// Generate Helper
VulkanTextureDescriptor VULKAN_EXPORT generateVulkanTextureDescriptor(const TextureDescriptor& descriptor);

// Convert Helper
VkFormat VULKAN_EXPORT ToVkFormat(TextureFormat format);
TextureFormat ToTextureFormat(VkFormat format);
VkImageType ToVkImageType(TextureType type);
TextureType ToTextureType(VkImageType type);
VkImageUsageFlags ToVkImageUsageFlags(TextureUsageFlags usages, TextureFormat format);
TextureUsageFlags ToTextureUsageFlags(VkImageUsageFlags usages);
VkAccessFlags ToVkAccessFlags(StorageTextureAccess access);
StorageTextureAccess ToStorageTextureAccess(VkAccessFlags access);
VkSampleCountFlagBits VULKAN_EXPORT ToVkSampleCountFlagBits(uint32_t count);
uint32_t ToSampleCount(VkSampleCountFlagBits flag);

// Utils
bool isSupportedVkFormat(VkFormat format);
VkImageLayout GenerateFinalImageLayout(VkImageUsageFlags usage);
// VkImageLayout GenerateFinalImageLayout(TextureUsageFlags usage);
VkAccessFlags GenerateAccessFlags(VkImageLayout layout);
VkPipelineStageFlags GenerateSrcPipelineStage(VkImageLayout layout);
VkPipelineStageFlags GenerateDstPipelineStage(VkImageLayout layout);

} // namespace jipu
