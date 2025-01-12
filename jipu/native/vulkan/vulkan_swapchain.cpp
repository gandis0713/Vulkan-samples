#include "vulkan_swapchain.h"

#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include "vulkan_queue.h"
#include "vulkan_surface.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

namespace
{

VkCompositeAlphaFlagBitsKHR getCompositeAlphaFlagBit(VkCompositeAlphaFlagsKHR supportedCompositeAlpha)
{
    std::array<VkCompositeAlphaFlagBitsKHR, 4> compositeAlphaFlagBits = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (const auto compositeAlphaFlagBit : compositeAlphaFlagBits)
    {
        if (supportedCompositeAlpha & compositeAlphaFlagBit)
        {
            return compositeAlphaFlagBit;
        }
    }

    throw std::runtime_error(fmt::format("Failed to find supported composite alpha flag bit. [{}]", supportedCompositeAlpha));

    return static_cast<VkCompositeAlphaFlagBitsKHR>(0x00000000);
}
} // namespace

VulkanSwapchainDescriptor generateVulkanSwapchainDescriptor(VulkanDevice* device, const SwapchainDescriptor& descriptor)
{
    VulkanSurface* surface = downcast(descriptor.surface);

    auto vulkanPhysicalDevice = device->getPhysicalDevice();
    VulkanSurfaceInfo surfaceInfo = vulkanPhysicalDevice->gatherSurfaceInfo(surface);

    // Check surface formats supports.
    auto surfaceFormatIter = std::find_if(surfaceInfo.formats.begin(),
                                          surfaceInfo.formats.end(),
                                          [textureFormat = descriptor.textureFormat, colorSpace = descriptor.colorSpace](const VkSurfaceFormatKHR& surfaceFormat) { return surfaceFormat.format == ToVkFormat(textureFormat) &&
                                                                                                                                                                           surfaceFormat.colorSpace == ToVkColorSpaceKHR(colorSpace); });
    if (surfaceFormatIter == surfaceInfo.formats.end())
    {
        throw std::runtime_error(fmt::format("{} texture format or/and {} color space are not supported.",
                                             static_cast<uint32_t>(descriptor.textureFormat),
                                             static_cast<uint32_t>(descriptor.colorSpace)));
    }
    const VkSurfaceFormatKHR surfaceFormat = *surfaceFormatIter;

    // Check surface present mode.
    auto presentModeIter = std::find(surfaceInfo.presentModes.begin(),
                                     surfaceInfo.presentModes.end(),
                                     ToVkPresentModeKHR(descriptor.presentMode));
    if (presentModeIter == surfaceInfo.presentModes.end())
    {
        throw std::runtime_error(fmt::format("{} present mode is not supported.", static_cast<uint32_t>(descriptor.presentMode)));
    }
    const VkPresentModeKHR presentMode = *presentModeIter;

    // Check surface capabilities.
    const VkSurfaceCapabilitiesKHR& surfaceCapabilities = surfaceInfo.capabilities;
    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
    {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    uint32_t width = descriptor.width;
    uint32_t height = descriptor.height;

    // If extent is invalid, use current extent.
    if (descriptor.width < surfaceCapabilities.minImageExtent.width ||
        descriptor.width > surfaceCapabilities.maxImageExtent.width ||
        descriptor.height < surfaceCapabilities.minImageExtent.height ||
        descriptor.height > surfaceCapabilities.maxImageExtent.height)
    {
        width = surfaceCapabilities.currentExtent.width;
        height = surfaceCapabilities.currentExtent.height;
    }

    VulkanSwapchainDescriptor vkdescriptor{
        .surface = surface
    };
    vkdescriptor.minImageCount = imageCount;
    vkdescriptor.imageFormat = surfaceFormat.format;
    vkdescriptor.imageColorSpace = surfaceFormat.colorSpace;
    vkdescriptor.imageExtent = { width, height };
    vkdescriptor.imageArrayLayers = 1;
    vkdescriptor.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // TODO: check sharing mode
    // if graphics and present queue family are difference.
    // {
    //     vkdescriptor.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //     vkdescriptor.queueFamilyIndices.resize(2);
    //     vkdescriptor.pQueueFamilyIndices = queueFamilyIndices;
    // }
    // else
    // {
    //     vkdescriptor.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // }

    vkdescriptor.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkdescriptor.queueFamilyIndices = {}; // Optional
    vkdescriptor.preTransform = surfaceCapabilities.currentTransform;
    vkdescriptor.compositeAlpha = getCompositeAlphaFlagBit(surfaceCapabilities.supportedCompositeAlpha);
    vkdescriptor.presentMode = presentMode;
    vkdescriptor.clipped = VK_TRUE;
    vkdescriptor.oldSwapchain = VK_NULL_HANDLE;
    vkdescriptor.queue = downcast(descriptor.queue);

    return vkdescriptor;
}

VulkanSwapchainTexture::VulkanSwapchainTexture(VulkanDevice* device, const VulkanTextureDescriptor& descriptor, const VulkanSwapchainTextureDescriptor& swapchainTextureDescriptor)
    : VulkanTexture(device, descriptor)
    , m_imageIndex(swapchainTextureDescriptor.imageIndex)
{
}

VulkanSwapchainTexture::~VulkanSwapchainTexture()
{
    if (m_semaphore != VK_NULL_HANDLE)
    {
        auto vulkanDevice = downcast(m_device);
        vulkanDevice->getDeleter()->safeDestroy(m_semaphore);
    }
}

uint32_t VulkanSwapchainTexture::getImageIndex() const
{
    return m_imageIndex;
}

void VulkanSwapchainTexture::setAcquireSemaphore(VkSemaphore semaphore)
{
    m_semaphore = semaphore;
}

VkSemaphore VulkanSwapchainTexture::getAcquireSemaphore() const
{
    return m_semaphore;
}

VulkanSwapchainTextureView::VulkanSwapchainTextureView(VulkanTexture* texture, const TextureViewDescriptor& descriptor)
    : VulkanTextureView(texture, descriptor)
{
}

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, const SwapchainDescriptor& descriptor) noexcept(false)
    : m_device(device)
    , m_descriptor(generateVulkanSwapchainDescriptor(device, descriptor))
{
    createSwapchain(m_descriptor);
}

uint32_t VulkanSwapchainTextureView::getImageIndex() const
{
    return downcast(m_texture)->getImageIndex();
}

VkSemaphore VulkanSwapchainTextureView::getAcquireSemaphore() const
{
    return downcast(m_texture)->getAcquireSemaphore();
}

VulkanSwapchain::~VulkanSwapchain()
{
    auto vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    /* do not delete VkImages from swapchain. */

    vkAPI.DestroySwapchainKHR(downcast(m_device)->getVkDevice(), m_swapchain, nullptr);
}

TextureFormat VulkanSwapchain::getTextureFormat() const
{
    return ToTextureFormat(m_descriptor.imageFormat);
}

uint32_t VulkanSwapchain::getWidth() const
{
    return m_descriptor.imageExtent.width;
}

uint32_t VulkanSwapchain::getHeight() const
{
    return m_descriptor.imageExtent.height;
}

void VulkanSwapchain::present()
{
    VulkanDevice* vulkanDevice = downcast(m_device);
    auto vulkanQueue = downcast(m_descriptor.queue);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    VulkanPresentInfo presentInfo{};
    presentInfo.signalSemaphore.push_back(downcast(m_textures[m_acquiredImageIndex].get())->getAcquireSemaphore());
    presentInfo.swapchains = { m_swapchain };
    presentInfo.imageIndices = { m_acquiredImageIndex };

    vulkanQueue->present(presentInfo);
}

void VulkanSwapchain::resize(uint32_t width, uint32_t height)
{
    auto newDescriptor = m_descriptor;
    newDescriptor.imageExtent = { width, height }; // TODO: check extent by surface capabilities.
    newDescriptor.oldSwapchain = m_swapchain;

    createSwapchain(newDescriptor);
}

Texture* VulkanSwapchain::acquireNextTexture()
{
    return m_textures[acquireNextImageIndex()].get();
}

TextureView* VulkanSwapchain::acquireNextTextureView()
{
    return m_textureViews[acquireNextImageIndex()].get();
}

void VulkanSwapchain::createSwapchain(const VulkanSwapchainDescriptor& descriptor)
{
    m_descriptor = descriptor;

    m_textureViews.clear();
    m_textures.clear();

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = m_descriptor.surface->getVkSurface();
    swapchainCreateInfo.minImageCount = m_descriptor.minImageCount;
    swapchainCreateInfo.imageFormat = m_descriptor.imageFormat;
    swapchainCreateInfo.imageColorSpace = m_descriptor.imageColorSpace;
    swapchainCreateInfo.imageExtent = m_descriptor.imageExtent;
    swapchainCreateInfo.imageArrayLayers = m_descriptor.imageArrayLayers;
    swapchainCreateInfo.imageUsage = m_descriptor.imageUsage;
    swapchainCreateInfo.imageSharingMode = m_descriptor.imageSharingMode;
    swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(descriptor.queueFamilyIndices.size());
    swapchainCreateInfo.pQueueFamilyIndices = m_descriptor.queueFamilyIndices.data();
    swapchainCreateInfo.preTransform = m_descriptor.preTransform;
    swapchainCreateInfo.compositeAlpha = m_descriptor.compositeAlpha;
    swapchainCreateInfo.presentMode = m_descriptor.presentMode;
    swapchainCreateInfo.clipped = m_descriptor.clipped;
    swapchainCreateInfo.oldSwapchain = m_descriptor.oldSwapchain;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    const VulkanAPI& vkAPI = m_device->vkAPI;
    if (vkAPI.CreateSwapchainKHR(m_device->getVkDevice(), &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain");
    }
    m_swapchain = swapchain;

    if (m_descriptor.oldSwapchain)
    {
        vkAPI.DestroySwapchainKHR(downcast(m_device)->getVkDevice(), m_descriptor.oldSwapchain, nullptr);
    }

    // get swapchain image.
    vkAPI.GetSwapchainImagesKHR(m_device->getVkDevice(), m_swapchain, &swapchainCreateInfo.minImageCount, nullptr);

    std::vector<VkImage> images{};
    images.resize(swapchainCreateInfo.minImageCount);
    vkAPI.GetSwapchainImagesKHR(m_device->getVkDevice(), m_swapchain, &swapchainCreateInfo.minImageCount, images.data());

    // create Textures by VkImage.
    for (auto index = 0; index < images.size(); ++index)
    {
        auto image = images[index];

        VulkanTextureDescriptor descriptor{};
        descriptor.imageType = VK_IMAGE_TYPE_2D;
        descriptor.extent = { swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height, 1 };
        descriptor.mipLevels = 1;
        descriptor.arrayLayers = 1;
        descriptor.format = swapchainCreateInfo.imageFormat;
        descriptor.tiling = VK_IMAGE_TILING_OPTIMAL;
        descriptor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        descriptor.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        descriptor.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        descriptor.samples = VK_SAMPLE_COUNT_1_BIT;
        descriptor.flags = 0; // Optional

        descriptor.image = image; // by swapchain image.
        descriptor.owner = VulkanTextureOwner::kSwapchain;

        VulkanSwapchainTextureDescriptor swapchainTextureDescriptor{};
        swapchainTextureDescriptor.imageIndex = index;

        std::unique_ptr<VulkanSwapchainTexture>
            texture = std::make_unique<VulkanSwapchainTexture>(m_device, descriptor, swapchainTextureDescriptor);
        m_textures.push_back(std::move(texture));
    }

    // create TextureViews
    for (std::unique_ptr<VulkanSwapchainTexture>& texture : m_textures)
    {
        TextureViewDescriptor descriptor{};
        descriptor.dimension = TextureViewDimension::k2D;
        descriptor.aspect = TextureAspectFlagBits::kColor;
        auto textureView = std::make_unique<VulkanSwapchainTextureView>(texture.get(), descriptor);
        m_textureViews.push_back(std::move(textureView));
    }
}

uint32_t VulkanSwapchain::acquireNextImageIndex()
{
    VulkanDevice* vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;

    auto semaphore = m_device->getSemaphorePool()->create();
    m_device->getInflightObjects()->standby(semaphore);
    m_device->getDeleter()->safeDestroy(semaphore);

    uint32_t acquireImageIndex = 0;
    VkResult result = vkAPI.AcquireNextImageKHR(vulkanDevice->getVkDevice(), m_swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &acquireImageIndex);
    if (result != VK_SUCCESS)
    {
        spdlog::error("Failed to acquire next image index. error: {}", static_cast<int32_t>(result));
    }

    setAcquireImageInfo(acquireImageIndex, semaphore);

    return acquireImageIndex;
}

void VulkanSwapchain::setAcquireImageInfo(const uint32_t imageIndex, VkSemaphore semaphore)
{
    if (m_textures.size() <= imageIndex)
    {
        throw std::runtime_error("Failed to set acquire image semaphore. Invalid image index.");
    }

    m_textures[imageIndex]->setAcquireSemaphore(semaphore);
    m_acquiredImageIndex = imageIndex;
}

VkSemaphore VulkanSwapchain::getAcquireSemaphore(const uint32_t imageIndex) const
{
    if (m_textures.size() <= imageIndex)
    {
        throw std::runtime_error("Failed to get acquire semaphore. Invalid image index.");
    }

    return m_textures[imageIndex]->getAcquireSemaphore();
}

uint32_t VulkanSwapchain::getAcquireImageIndex() const
{
    return m_acquiredImageIndex;
}

VkSwapchainKHR VulkanSwapchain::getVkSwapchainKHR() const
{
    return m_swapchain;
}

} // namespace jipu
