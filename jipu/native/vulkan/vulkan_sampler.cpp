#include "vulkan_sampler.h"
#include "vulkan_device.h"
#include "vulkan_physical_device.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

VulkanSampler::VulkanSampler(VulkanDevice& device, const SamplerDescriptor& descriptor)
    : m_device(device)
{
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = ToVkFilter(descriptor.magFilter);
    createInfo.minFilter = ToVkFilter(descriptor.minFilter);
    createInfo.mipmapMode = ToVkSamplerMipmapMode(descriptor.mipmapFilter);
    createInfo.addressModeU = ToVkSamplerAddressMode(descriptor.addressModeU);
    createInfo.addressModeV = ToVkSamplerAddressMode(descriptor.addressModeV);
    createInfo.addressModeW = ToVkSamplerAddressMode(descriptor.addressModeW);
    createInfo.anisotropyEnable = VK_FALSE;
    createInfo.maxAnisotropy = 1.0f;
    // check it from physical device features.
    {
        // createInfo.anisotropyEnable = VK_TRUE;
        // createInfo.maxAnisotropy = downcast(m_device.getPhysicalDevice()).getVulkanPhysicalDeviceInfo().physicalDeviceProperties.limits.maxSamplerAnisotropy;
    }
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = descriptor.lodMin;
    createInfo.maxLod = descriptor.lodMax;

    const VulkanAPI& vkAPI = device.vkAPI;
    VkResult result = vkAPI.CreateSampler(device.getVkDevice(), &createInfo, nullptr, &m_sampler);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create sampler. {}", static_cast<int32_t>(result)));
    }
}

VulkanSampler::~VulkanSampler()
{
    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    vkAPI.DestroySampler(vulkanDevice.getVkDevice(), m_sampler, nullptr);
}

VkSampler VulkanSampler::getVkSampler() const
{
    return m_sampler;
}

// Convert Helper
VkSamplerAddressMode ToVkSamplerAddressMode(AddressMode mode)
{
    switch (mode)
    {
    case AddressMode::kRepeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::kMirrorRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::kClampToEdge:
    default:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

AddressMode ToAddressMode(VkSamplerAddressMode mode)
{
    switch (mode)
    {
    case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
        return AddressMode::kClampToEdge;
    case VK_SAMPLER_ADDRESS_MODE_REPEAT:
        return AddressMode::kRepeat;
    case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
        return AddressMode::kMirrorRepeat;
    default:
        throw std::runtime_error(fmt::format("{} address mode does not support.", static_cast<int32_t>(mode)));
        return AddressMode::kClampToEdge;
    }
}

VkFilter ToVkFilter(FilterMode mode)
{
    switch (mode)
    {
    case FilterMode::kLinear:
        return VK_FILTER_LINEAR;
    case FilterMode::kNearest:
    default:
        return VK_FILTER_NEAREST;
    }
}

FilterMode ToFilterMode(VkFilter filter)
{
    switch (filter)
    {
    case VK_FILTER_NEAREST:
        return FilterMode::kNearest;
    case VK_FILTER_LINEAR:
        return FilterMode::kLinear;
    default:
        throw std::runtime_error(fmt::format("{} mode does not support.", static_cast<int32_t>(filter)));
        return FilterMode::kNearest;
    }
}

VkSamplerMipmapMode ToVkSamplerMipmapMode(MipmapFilterMode mode)
{
    switch (mode)
    {
    case MipmapFilterMode::kLinear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    case MipmapFilterMode::kNearest:
    default:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

MipmapFilterMode ToMipmapFilterMode(VkSamplerMipmapMode mode)
{
    switch (mode)
    {
    case VK_SAMPLER_MIPMAP_MODE_NEAREST:
        return MipmapFilterMode::kNearest;
    case VK_SAMPLER_MIPMAP_MODE_LINEAR:
        return MipmapFilterMode::kLinear;
    default:
        throw std::runtime_error(fmt::format("{} mode does not support.", static_cast<int32_t>(mode)));
        return MipmapFilterMode::kNearest;
    }
}

}; // namespace jipu
