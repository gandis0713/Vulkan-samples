#pragma once

#include "swapchain.h"

#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include "jipu/common/cast.h"

#include <memory>
#include <vector>

namespace jipu
{

class VulkanDevice;
class VulkanQueue;
class VulkanTexture;
class VulkanSurface;
class VulkanTextureView;

struct VulkanSwapchainDescriptor
{
    const void* next = nullptr;
    VkSwapchainCreateFlagsKHR flags = 0u;
    VulkanSurface* surface = nullptr;
    uint32_t minImageCount = 0;
    VkFormat imageFormat;
    VkColorSpaceKHR imageColorSpace;
    VkExtent2D imageExtent;
    uint32_t imageArrayLayers;
    VkImageUsageFlags imageUsage;
    VkSharingMode imageSharingMode;
    std::vector<uint32_t> queueFamilyIndices{};
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkCompositeAlphaFlagBitsKHR compositeAlpha;
    VkPresentModeKHR presentMode;
    VkBool32 clipped;
    VkSwapchainKHR oldSwapchain;
    VulkanQueue* queue = nullptr;
};

struct VulkanSwapchainTextureDescriptor
{
    uint32_t imageIndex = 0;
};

struct VulkanPresentInfo
{
    std::vector<VkSemaphore> signalSemaphore{};
    std::vector<VkSemaphore> waitSemaphores{};
    std::vector<VkSwapchainKHR> swapchains{};
    std::vector<uint32_t> imageIndices{};
};

class VULKAN_EXPORT VulkanSwapchainTexture : public VulkanTexture
{
public:
    VulkanSwapchainTexture() = delete;
    VulkanSwapchainTexture(VulkanDevice* device, const VulkanTextureDescriptor&, const VulkanSwapchainTextureDescriptor&);
    ~VulkanSwapchainTexture() override = default;

public:
    uint32_t getImageIndex() const;

    void setAcquireSemaphore(VkSemaphore semaphore);
    VkSemaphore getAcquireSemaphore() const;

private:
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
    uint32_t m_imageIndex = 0u;
};
DOWN_CAST(VulkanSwapchainTexture, VulkanTexture);

class VULKAN_EXPORT VulkanSwapchainTextureView : public VulkanTextureView
{
public:
    VulkanSwapchainTextureView() = delete;
    VulkanSwapchainTextureView(VulkanTexture* texture, const TextureViewDescriptor& descriptor);
    ~VulkanSwapchainTextureView() override = default;

public:
    uint32_t getImageIndex() const;
    VkSemaphore getAcquireSemaphore() const;
};
DOWN_CAST(VulkanSwapchainTextureView, VulkanTextureView);

class VULKAN_EXPORT VulkanSwapchain : public Swapchain
{
public:
    VulkanSwapchain() = delete;
    VulkanSwapchain(VulkanDevice* device, const SwapchainDescriptor& descriptor) noexcept(false);
    VulkanSwapchain(VulkanDevice* device, const VulkanSwapchainDescriptor& descriptor) noexcept(false);
    ~VulkanSwapchain() override;

    VulkanSwapchain(const Swapchain&) = delete;
    VulkanSwapchain& operator=(const Swapchain&) = delete;

    TextureFormat getTextureFormat() const override;
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;

    void present() override;
    Texture* acquireNextTexture() override;
    TextureView* acquireNextTextureView() override;

public:
    VkSwapchainKHR getVkSwapchainKHR() const;

private:
    uint32_t acquireNextImageIndex();

    void setAcquireImageInfo(const uint32_t imageIndex, VkSemaphore semaphore);

    VkSemaphore getAcquireSemaphore(const uint32_t imageIndex) const;
    uint32_t getAcquireImageIndex() const;

private:
    VulkanDevice* m_device = nullptr;
    const VulkanSwapchainDescriptor m_descriptor;

    std::vector<std::unique_ptr<VulkanSwapchainTexture>> m_textures{};
    std::vector<std::unique_ptr<VulkanSwapchainTextureView>> m_textureViews{};

private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    uint32_t m_acquiredImageIndex = 0u;
};

DOWN_CAST(VulkanSwapchain, Swapchain);

// Generate Helper
VulkanSwapchainDescriptor VULKAN_EXPORT generateVulkanSwapchainDescriptor(VulkanDevice* device, const SwapchainDescriptor& descriptor);

} // namespace jipu
