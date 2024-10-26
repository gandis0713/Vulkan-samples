#include "vulkan_texture.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_resource_allocator.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanTextureDescriptor generateVulkanTextureDescriptor(const TextureDescriptor& descriptor)
{
    VulkanTextureDescriptor vkdescriptor{};

    vkdescriptor.imageType = ToVkImageType(descriptor.type);
    vkdescriptor.extent.width = descriptor.width;
    vkdescriptor.extent.height = descriptor.height;
    vkdescriptor.extent.depth = descriptor.depth;
    vkdescriptor.mipLevels = descriptor.mipLevels;
    vkdescriptor.arrayLayers = 1;
    vkdescriptor.format = ToVkFormat(descriptor.format);
    vkdescriptor.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkdescriptor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkdescriptor.usage = ToVkImageUsageFlags(descriptor.usage);
    vkdescriptor.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkdescriptor.samples = ToVkSampleCountFlagBits(descriptor.sampleCount);
    vkdescriptor.flags = 0;

    return vkdescriptor;
}

VulkanTexture::VulkanTexture(VulkanDevice& device, const TextureDescriptor& descriptor)
    : VulkanTexture(device, generateVulkanTextureDescriptor(descriptor))
{
}

VulkanTexture::VulkanTexture(VulkanDevice& device, const VulkanTextureDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    if (m_descriptor.extent.width == 0 || m_descriptor.extent.height == 0 || m_descriptor.extent.depth == 0)
    {
        throw std::runtime_error("Texture size must be greater than 0.");
    }

    if (m_descriptor.usage == 0u)
    {
        throw std::runtime_error("Texture usage must not be undefined.");
    }

    if (m_descriptor.format == VK_FORMAT_UNDEFINED)
    {
        throw std::runtime_error("Texture format must not be undefined.");
    }

    if (m_descriptor.image == VK_NULL_HANDLE)
    {
        VkImageCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.imageType = m_descriptor.imageType;
        createInfo.extent.width = m_descriptor.extent.width;
        createInfo.extent.height = m_descriptor.extent.height;
        createInfo.extent.depth = m_descriptor.extent.depth;
        createInfo.mipLevels = m_descriptor.mipLevels;
        createInfo.arrayLayers = m_descriptor.arrayLayers;
        createInfo.format = m_descriptor.format;
        createInfo.tiling = m_descriptor.tiling;
        createInfo.initialLayout = m_descriptor.initialLayout;
        createInfo.usage = m_descriptor.usage;
        createInfo.sharingMode = m_descriptor.sharingMode;
        createInfo.samples = m_descriptor.samples;
        createInfo.flags = m_descriptor.flags;

        auto& vulkanResourceAllocator = device.getResourceAllocator();
        m_resource = vulkanResourceAllocator.createTexture(createInfo);

        m_owner = Owner::User;
    }
    else
    {
        m_resource.image = m_descriptor.image;
        m_owner = Owner::Swapchain;
    }
}

VulkanTexture::~VulkanTexture()
{
    if (m_owner == Owner::User)
    {
        auto& vulkanResourceAllocator = downcast(m_device).getResourceAllocator();
        vulkanResourceAllocator.destroyTexture(m_resource);
    }
}

std::unique_ptr<TextureView> VulkanTexture::createTextureView(const TextureViewDescriptor& descriptor)
{
    return std::make_unique<VulkanTextureView>(*this, descriptor);
}

TextureType VulkanTexture::getType() const
{
    return ToTextureType(m_descriptor.imageType);
}

TextureFormat VulkanTexture::getFormat() const
{
    return ToTextureFormat(m_descriptor.format);
}

TextureUsageFlags VulkanTexture::getUsage() const
{
    return ToTextureUsageFlags(m_descriptor.usage);
}

uint32_t VulkanTexture::getWidth() const
{
    return m_descriptor.extent.width;
}

uint32_t VulkanTexture::getHeight() const
{
    return m_descriptor.extent.height;
}

uint32_t VulkanTexture::getDepth() const
{
    return m_descriptor.extent.depth;
}

uint32_t VulkanTexture::getMipLevels() const
{
    return m_descriptor.mipLevels;
}

uint32_t VulkanTexture::getSampleCount() const
{
    return ToSampleCount(m_descriptor.samples);
}

VulkanDevice& VulkanTexture::getDevice() const
{
    return m_device;
}

VkImage VulkanTexture::getVkImage() const
{
    return m_resource.image;
}

VkImageLayout VulkanTexture::getFinalLayout() const
{
    switch (m_owner)
    {
    case Owner::Swapchain:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
    case Owner::User:
        return GenerateFinalImageLayout(m_descriptor.usage);
    }
}

void VulkanTexture::setPipelineBarrier(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange range)
{
    if (commandBuffer == VK_NULL_HANDLE)
        throw std::runtime_error("Command buffer is null handle to set pipeline barrier in texture.");

    if (oldLayout == newLayout)
    {
        spdlog::debug("old layout and new layout are same.");
        return;
    }

    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    // set Image Memory Barrier
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = VK_NULL_HANDLE;
    barrier.srcAccessMask = GenerateAccessFlags(oldLayout);
    barrier.dstAccessMask = GenerateAccessFlags(newLayout);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = m_resource.image;
    barrier.subresourceRange = range;

    VkPipelineStageFlags srcStage = GenerateSrcPipelineStage(oldLayout);
    VkPipelineStageFlags dstStage = GenerateDstPipelineStage(newLayout);

    setPipelineBarrier(commandBuffer, srcStage, dstStage, barrier);
}

void VulkanTexture::setPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkImageMemoryBarrier barrier)
{
    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    vkAPI.CmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VulkanTexture::Owner VulkanTexture::getOwner() const
{
    return m_owner;
}

// Convert Helper

VkFormat ToVkFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::kUndefined:
        return VK_FORMAT_UNDEFINED;
    case TextureFormat::kR8Unorm:
        return VK_FORMAT_R8_UNORM;
    case TextureFormat::kR8Snorm:
        return VK_FORMAT_R8_SNORM;
    case TextureFormat::kR8Uint:
        return VK_FORMAT_R8_UINT;
    case TextureFormat::kR8Sint:
        return VK_FORMAT_R8_SINT;
    case TextureFormat::kR16Uint:
        return VK_FORMAT_R16_UINT;
    case TextureFormat::kR16Sint:
        return VK_FORMAT_R16_SINT;
    case TextureFormat::kR16Float:
        return VK_FORMAT_R16_SFLOAT;
    case TextureFormat::kRG8Unorm:
        return VK_FORMAT_R8G8_UNORM;
    case TextureFormat::kRG8Snorm:
        return VK_FORMAT_R8G8_SNORM;
    case TextureFormat::kRG8Uint:
        return VK_FORMAT_R8G8_UINT;
    case TextureFormat::kRG8Sint:
        return VK_FORMAT_R8G8_SINT;
    case TextureFormat::kR32Float:
        return VK_FORMAT_R32_SFLOAT;
    case TextureFormat::kR32Uint:
        return VK_FORMAT_R32_UINT;
    case TextureFormat::kR32Sint:
        return VK_FORMAT_R32_SINT;
    case TextureFormat::kRG16Uint:
        return VK_FORMAT_R16G16_UINT;
    case TextureFormat::kRG16Sint:
        return VK_FORMAT_R16G16_SINT;
    case TextureFormat::kRG16Float:
        return VK_FORMAT_R16G16_SFLOAT;
    case TextureFormat::kRGBA8Unorm:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::kRGBA8UnormSrgb:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case TextureFormat::kRGBA8Snorm:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case TextureFormat::kRGBA8Uint:
        return VK_FORMAT_R8G8B8A8_UINT;
    case TextureFormat::kRGBA8Sint:
        return VK_FORMAT_R8G8B8A8_SINT;
    case TextureFormat::kBGRA8Unorm:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case TextureFormat::kBGRA8UnormSrgb:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case TextureFormat::kRGB10A2Uint:
        return VK_FORMAT_A2B10G10R10_UINT_PACK32;
    case TextureFormat::kRGB10A2Unorm:
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case TextureFormat::kRG11B10Ufloat:
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case TextureFormat::kRGB9E5Ufloat:
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
    case TextureFormat::kRG32Float:
        return VK_FORMAT_R32G32_SFLOAT;
    case TextureFormat::kRG32Uint:
        return VK_FORMAT_R32G32_UINT;
    case TextureFormat::kRG32Sint:
        return VK_FORMAT_R32G32_SINT;
    case TextureFormat::kRGBA16Uint:
        return VK_FORMAT_R16G16B16A16_UINT;
    case TextureFormat::kRGBA16Sint:
        return VK_FORMAT_R16G16B16A16_SINT;
    case TextureFormat::kRGBA16Float:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TextureFormat::kRGBA32Float:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case TextureFormat::kRGBA32Uint:
        return VK_FORMAT_R32G32B32A32_UINT;
    case TextureFormat::kRGBA32Sint:
        return VK_FORMAT_R32G32B32A32_SINT;
    case TextureFormat::kStencil8:
        return VK_FORMAT_S8_UINT;
    case TextureFormat::kDepth16Unorm:
        return VK_FORMAT_D16_UNORM;
    case TextureFormat::kDepth24Plus:
        return VK_FORMAT_X8_D24_UNORM_PACK32;
    case TextureFormat::kDepth24PlusStencil8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case TextureFormat::kDepth32Float:
        return VK_FORMAT_D32_SFLOAT;
    case TextureFormat::kBC1RGBAUnorm:
        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case TextureFormat::kBC1RGBAUnormSrgb:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case TextureFormat::kBC2RGBAUnorm:
        return VK_FORMAT_BC2_UNORM_BLOCK;
    case TextureFormat::kBC2RGBAUnormSrgb:
        return VK_FORMAT_BC2_SRGB_BLOCK;
    case TextureFormat::kBC3RGBAUnorm:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    case TextureFormat::kBC3RGBAUnormSrgb:
        return VK_FORMAT_BC3_SRGB_BLOCK;
    case TextureFormat::kBC4RUnorm:
        return VK_FORMAT_BC4_UNORM_BLOCK;
    case TextureFormat::kBC4RSnorm:
        return VK_FORMAT_BC4_SNORM_BLOCK;
    case TextureFormat::kBC5RGUnorm:
        return VK_FORMAT_BC5_UNORM_BLOCK;
    case TextureFormat::kBC5RGSnorm:
        return VK_FORMAT_BC5_SNORM_BLOCK;
    case TextureFormat::kBC6HRGBUfloat:
        return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case TextureFormat::kBC6HRGBFloat:
        return VK_FORMAT_BC6H_SFLOAT_BLOCK;
    case TextureFormat::kBC7RGBAUnorm:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case TextureFormat::kBC7RGBAUnormSrgb:
        return VK_FORMAT_BC7_SRGB_BLOCK;
    case TextureFormat::kETC2RGB8Unorm:
        return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case TextureFormat::kETC2RGB8UnormSrgb:
        return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
    case TextureFormat::kETC2RGB8A1Unorm:
        return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
    case TextureFormat::kETC2RGB8A1UnormSrgb:
        return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
    case TextureFormat::kETC2RGBA8Unorm:
        return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    case TextureFormat::kETC2RGBA8UnormSrgb:
        return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
    case TextureFormat::kEACR11Unorm:
        return VK_FORMAT_EAC_R11_UNORM_BLOCK;
    case TextureFormat::kEACR11Snorm:
        return VK_FORMAT_EAC_R11_SNORM_BLOCK;
    case TextureFormat::kEACRG11Unorm:
        return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
    case TextureFormat::kEACRG11Snorm:
        return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
    case TextureFormat::kASTC4x4Unorm:
        return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
    case TextureFormat::kASTC4x4UnormSrgb:
        return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
    case TextureFormat::kASTC5x4Unorm:
        return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
    case TextureFormat::kASTC5x4UnormSrgb:
        return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
    case TextureFormat::kASTC5x5Unorm:
        return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
    case TextureFormat::kASTC5x5UnormSrgb:
        return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
    case TextureFormat::kASTC6x5Unorm:
        return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
    case TextureFormat::kASTC6x5UnormSrgb:
        return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
    case TextureFormat::kASTC6x6Unorm:
        return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
    case TextureFormat::kASTC6x6UnormSrgb:
        return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
    case TextureFormat::kASTC8x5Unorm:
        return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
    case TextureFormat::kASTC8x5UnormSrgb:
        return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
    case TextureFormat::kASTC8x6Unorm:
        return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
    case TextureFormat::kASTC8x6UnormSrgb:
        return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
    case TextureFormat::kASTC8x8Unorm:
        return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
    case TextureFormat::kASTC8x8UnormSrgb:
        return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
    case TextureFormat::kASTC10x5Unorm:
        return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
    case TextureFormat::kASTC10x5UnormSrgb:
        return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
    case TextureFormat::kASTC10x6Unorm:
        return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
    case TextureFormat::kASTC10x6UnormSrgb:
        return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
    case TextureFormat::kASTC10x8Unorm:
        return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
    case TextureFormat::kASTC10x8UnormSrgb:
        return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
    case TextureFormat::kASTC10x10Unorm:
        return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
    case TextureFormat::kASTC10x10UnormSrgb:
        return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
    case TextureFormat::kASTC12x10Unorm:
        return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
    case TextureFormat::kASTC12x10UnormSrgb:
        return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
    case TextureFormat::kASTC12x12Unorm:
        return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
    case TextureFormat::kASTC12x12UnormSrgb:
        return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    case TextureFormat::kR16Unorm:
        return VK_FORMAT_R16_UNORM;
    case TextureFormat::kRG16Unorm:
        return VK_FORMAT_R16G16_UNORM;
    case TextureFormat::kRGBA16Unorm:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case TextureFormat::kR16Snorm:
        return VK_FORMAT_R16_SNORM;
    case TextureFormat::kRG16Snorm:
        return VK_FORMAT_R16G16_SNORM;
    case TextureFormat::kRGBA16Snorm:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case TextureFormat::kR8BG8Biplanar420Unorm:
        return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    case TextureFormat::kR10X6BG10X6Biplanar420Unorm:
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
    case TextureFormat::kR8BG8A8Triplanar420Unorm:
        return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    case TextureFormat::kR8BG8Biplanar422Unorm:
        return VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;
    case TextureFormat::kR8BG8Biplanar444Unorm:
        return VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
    case TextureFormat::kR10X6BG10X6Biplanar422Unorm:
        return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
    case TextureFormat::kR10X6BG10X6Biplanar444Unorm:
        return VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
    case TextureFormat::kForce32:
    default:
        throw std::runtime_error(fmt::format("{} format does not support.", static_cast<uint32_t>(format)));
    }
}

TextureFormat ToTextureFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return TextureFormat::kUndefined;
    case VK_FORMAT_R8_UNORM:
        return TextureFormat::kR8Unorm;
    case VK_FORMAT_R8_SNORM:
        return TextureFormat::kR8Snorm;
    case VK_FORMAT_R8_UINT:
        return TextureFormat::kR8Uint;
    case VK_FORMAT_R8_SINT:
        return TextureFormat::kR8Sint;
    case VK_FORMAT_R16_UINT:
        return TextureFormat::kR16Uint;
    case VK_FORMAT_R16_SINT:
        return TextureFormat::kR16Sint;
    case VK_FORMAT_R16_SFLOAT:
        return TextureFormat::kR16Float;
    case VK_FORMAT_R8G8_UNORM:
        return TextureFormat::kRG8Unorm;
    case VK_FORMAT_R8G8_SNORM:
        return TextureFormat::kRG8Snorm;
    case VK_FORMAT_R8G8_UINT:
        return TextureFormat::kRG8Uint;
    case VK_FORMAT_R8G8_SINT:
        return TextureFormat::kRG8Sint;
    case VK_FORMAT_R32_SFLOAT:
        return TextureFormat::kR32Float;
    case VK_FORMAT_R32_UINT:
        return TextureFormat::kR32Uint;
    case VK_FORMAT_R32_SINT:
        return TextureFormat::kR32Sint;
    case VK_FORMAT_R16G16_UINT:
        return TextureFormat::kRG16Uint;
    case VK_FORMAT_R16G16_SINT:
        return TextureFormat::kRG16Sint;
    case VK_FORMAT_R16G16_SFLOAT:
        return TextureFormat::kRG16Float;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return TextureFormat::kRGBA8Unorm;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return TextureFormat::kRGBA8UnormSrgb;
    case VK_FORMAT_R8G8B8A8_SNORM:
        return TextureFormat::kRGBA8Snorm;
    case VK_FORMAT_R8G8B8A8_UINT:
        return TextureFormat::kRGBA8Uint;
    case VK_FORMAT_R8G8B8A8_SINT:
        return TextureFormat::kRGBA8Sint;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return TextureFormat::kBGRA8Unorm;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return TextureFormat::kBGRA8UnormSrgb;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        return TextureFormat::kRGB10A2Uint;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return TextureFormat::kRGB10A2Unorm;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return TextureFormat::kRG11B10Ufloat;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return TextureFormat::kRGB9E5Ufloat;
    case VK_FORMAT_R32G32_SFLOAT:
        return TextureFormat::kRG32Float;
    case VK_FORMAT_R32G32_UINT:
        return TextureFormat::kRG32Uint;
    case VK_FORMAT_R32G32_SINT:
        return TextureFormat::kRG32Sint;
    case VK_FORMAT_R16G16B16A16_UINT:
        return TextureFormat::kRGBA16Uint;
    case VK_FORMAT_R16G16B16A16_SINT:
        return TextureFormat::kRGBA16Sint;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return TextureFormat::kRGBA16Float;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return TextureFormat::kRGBA32Float;
    case VK_FORMAT_R32G32B32A32_UINT:
        return TextureFormat::kRGBA32Uint;
    case VK_FORMAT_R32G32B32A32_SINT:
        return TextureFormat::kRGBA32Sint;
    case VK_FORMAT_S8_UINT:
        return TextureFormat::kStencil8;
    case VK_FORMAT_D16_UNORM:
        return TextureFormat::kDepth16Unorm;
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return TextureFormat::kDepth24Plus;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return TextureFormat::kDepth24PlusStencil8;
    case VK_FORMAT_D32_SFLOAT:
        return TextureFormat::kDepth32Float;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return TextureFormat::kBC1RGBAUnorm;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return TextureFormat::kBC1RGBAUnormSrgb;
    case VK_FORMAT_BC2_UNORM_BLOCK:
        return TextureFormat::kBC2RGBAUnorm;
    case VK_FORMAT_BC2_SRGB_BLOCK:
        return TextureFormat::kBC2RGBAUnormSrgb;
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return TextureFormat::kBC3RGBAUnorm;
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return TextureFormat::kBC3RGBAUnormSrgb;
    case VK_FORMAT_BC4_UNORM_BLOCK:
        return TextureFormat::kBC4RUnorm;
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return TextureFormat::kBC4RSnorm;
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return TextureFormat::kBC5RGUnorm;
    case VK_FORMAT_BC5_SNORM_BLOCK:
        return TextureFormat::kBC5RGSnorm;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        return TextureFormat::kBC6HRGBUfloat;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return TextureFormat::kBC6HRGBFloat;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return TextureFormat::kBC7RGBAUnorm;
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return TextureFormat::kBC7RGBAUnormSrgb;
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        return TextureFormat::kETC2RGB8Unorm;
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        return TextureFormat::kETC2RGB8UnormSrgb;
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        return TextureFormat::kETC2RGB8A1Unorm;
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
        return TextureFormat::kETC2RGB8A1UnormSrgb;
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        return TextureFormat::kETC2RGBA8Unorm;
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
        return TextureFormat::kETC2RGBA8UnormSrgb;
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
        return TextureFormat::kEACR11Unorm;
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
        return TextureFormat::kEACR11Snorm;
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
        return TextureFormat::kEACRG11Unorm;
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
        return TextureFormat::kEACRG11Snorm;
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        return TextureFormat::kASTC4x4Unorm;
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return TextureFormat::kASTC4x4UnormSrgb;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        return TextureFormat::kASTC5x4Unorm;
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        return TextureFormat::kASTC5x4UnormSrgb;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        return TextureFormat::kASTC5x5Unorm;
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        return TextureFormat::kASTC5x5UnormSrgb;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        return TextureFormat::kASTC6x5Unorm;
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        return TextureFormat::kASTC6x5UnormSrgb;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        return TextureFormat::kASTC6x6Unorm;
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        return TextureFormat::kASTC6x6UnormSrgb;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        return TextureFormat::kASTC8x5Unorm;
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        return TextureFormat::kASTC8x5UnormSrgb;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        return TextureFormat::kASTC8x6Unorm;
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        return TextureFormat::kASTC8x6UnormSrgb;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        return TextureFormat::kASTC8x8Unorm;
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        return TextureFormat::kASTC8x8UnormSrgb;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        return TextureFormat::kASTC10x5Unorm;
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        return TextureFormat::kASTC10x5UnormSrgb;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        return TextureFormat::kASTC10x6Unorm;
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        return TextureFormat::kASTC10x6UnormSrgb;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        return TextureFormat::kASTC10x8Unorm;
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        return TextureFormat::kASTC10x8UnormSrgb;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        return TextureFormat::kASTC10x10Unorm;
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        return TextureFormat::kASTC10x10UnormSrgb;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        return TextureFormat::kASTC12x10Unorm;
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        return TextureFormat::kASTC12x10UnormSrgb;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        return TextureFormat::kASTC12x12Unorm;
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        return TextureFormat::kASTC12x12UnormSrgb;
    case VK_FORMAT_R16_UNORM:
        return TextureFormat::kR16Unorm;
    case VK_FORMAT_R16G16_UNORM:
        return TextureFormat::kRG16Unorm;
    case VK_FORMAT_R16G16B16A16_UNORM:
        return TextureFormat::kRGBA16Unorm;
    case VK_FORMAT_R16_SNORM:
        return TextureFormat::kR16Snorm;
    case VK_FORMAT_R16G16_SNORM:
        return TextureFormat::kRG16Snorm;
    case VK_FORMAT_R16G16B16A16_SNORM:
        return TextureFormat::kRGBA16Snorm;
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        return TextureFormat::kR8BG8Biplanar420Unorm;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        return TextureFormat::kR10X6BG10X6Biplanar420Unorm;
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        return TextureFormat::kR8BG8A8Triplanar420Unorm;
    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
        return TextureFormat::kR8BG8Biplanar422Unorm;
    case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
        return TextureFormat::kR8BG8Biplanar444Unorm;
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        return TextureFormat::kR10X6BG10X6Biplanar422Unorm;
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        return TextureFormat::kR10X6BG10X6Biplanar444Unorm;
    default:
        throw std::runtime_error(fmt::format("{} format does not support.", static_cast<uint32_t>(format)));
    }
}

VkImageType ToVkImageType(TextureType type)
{
    switch (type)
    {
    case TextureType::k1D:
        return VK_IMAGE_TYPE_1D;
    case TextureType::k2D:
        return VK_IMAGE_TYPE_2D;
    case TextureType::k3D:
        return VK_IMAGE_TYPE_3D;
    default:
        throw std::runtime_error(fmt::format("{} type does not support.", static_cast<uint32_t>(type)));
    }
}

TextureType ToTextureType(VkImageType type)
{
    switch (type)
    {
    case VK_IMAGE_TYPE_1D:
        return TextureType::k1D;
    case VK_IMAGE_TYPE_2D:
        return TextureType::k2D;
    case VK_IMAGE_TYPE_3D:
        return TextureType::k3D;
    default:
        throw std::runtime_error(fmt::format("{} type does not support.", static_cast<uint32_t>(type)));
    }
}

TextureUsageFlags ToTextureUsageFlags(VkImageUsageFlags usages)
{
    TextureUsageFlags flags = 0u;

    if (usages & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        flags |= TextureUsageFlagBits::kCopySrc;
    }
    if (usages & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        flags |= TextureUsageFlagBits::kCopyDst;
    }
    if (usages & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        flags |= TextureUsageFlagBits::kTextureBinding;
    }
    if (usages & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        flags |= TextureUsageFlagBits::kStorageBinding;
    }
    if (usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        flags |= TextureUsageFlagBits::kDepthStencil;
    }
    if (usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        flags |= TextureUsageFlagBits::kColorAttachment;
    }

    return flags;
}

VkImageUsageFlags ToVkImageUsageFlags(TextureUsageFlags usages)
{
    VkImageUsageFlags flags = 0u;

    if (usages & TextureUsageFlagBits::kCopySrc)
    {
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (usages & TextureUsageFlagBits::kCopyDst)
    {
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (usages & TextureUsageFlagBits::kTextureBinding)
    {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (usages & TextureUsageFlagBits::kStorageBinding)
    {
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (usages & TextureUsageFlagBits::kDepthStencil)
    {
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (usages & TextureUsageFlagBits::kColorAttachment)
    {
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    return flags;
}

VkSampleCountFlagBits ToVkSampleCountFlagBits(uint32_t count)
{
    return count <= 1 ? VK_SAMPLE_COUNT_1_BIT : VK_SAMPLE_COUNT_4_BIT;
}

uint32_t ToSampleCount(VkSampleCountFlagBits flag)
{
    if (flag == 0u)
    {
        throw std::runtime_error("sample count flag is undefined.");
    }

    return flag == VK_SAMPLE_COUNT_1_BIT ? 1 : 4;
}

// Utiles
bool isSupportedVkFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
    case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        return true;
    default:
        return false;
    }
}

VkImageLayout GenerateFinalImageLayout(VkImageUsageFlags usage)
{
    if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT || usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        return VK_IMAGE_LAYOUT_GENERAL;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

// VkImageLayout GenerateFinalImageLayout(TextureUsageFlags usage)
// {
//     if (usage & TextureUsageFlagBits::kTextureBinding)
//     {
//         return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//     }
//     if (usage & TextureUsageFlagBits::kColorAttachment)
//     {
//         return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//     }
//     if (usage & TextureUsageFlagBits::kStorageBinding)
//     {
//         return VK_IMAGE_LAYOUT_GENERAL;
//     }
//     if (usage & TextureUsageFlagBits::kDepthStencil)
//     {
//         return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//     }
//     if (usage & TextureUsageFlagBits::kCopySrc)
//     {
//         return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//     }
//     if (usage & TextureUsageFlagBits::kCopyDst)
//     {
//         return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//     }

//     return VK_IMAGE_LAYOUT_UNDEFINED;
// }

VkAccessFlags GenerateAccessFlags(VkImageLayout layout)
{
    VkAccessFlags accessFlags = 0x0u;

    switch (layout)
    {
    default:
    case VK_IMAGE_LAYOUT_UNDEFINED:
        accessFlags = VK_ACCESS_NONE;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        accessFlags = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        accessFlags = VK_ACCESS_SHADER_READ_BIT;
        break;
    }

    return accessFlags;
}

VkPipelineStageFlags GenerateSrcPipelineStage(VkImageLayout layout)
{
    VkPipelineStageFlags pipelineStage = 0x0u;

    switch (layout)
    {
    default:
    case VK_IMAGE_LAYOUT_UNDEFINED:
        pipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        pipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        pipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    }

    return pipelineStage;
}

VkPipelineStageFlags GenerateDstPipelineStage(VkImageLayout layout)
{
    VkPipelineStageFlags pipelineStage = 0x0u;

    switch (layout)
    {
    default:
    case VK_IMAGE_LAYOUT_UNDEFINED:
        pipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        pipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        pipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    }

    return pipelineStage;
}

} // namespace jipu
