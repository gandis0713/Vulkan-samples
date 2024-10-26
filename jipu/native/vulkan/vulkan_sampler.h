#pragma once

#include "jipu/common/cast.h"
#include "sampler.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

class VulkanDevice;
class VULKAN_EXPORT VulkanSampler : public Sampler
{
public:
    VulkanSampler() = delete;
    VulkanSampler(VulkanDevice& device, const SamplerDescriptor& descriptor);
    ~VulkanSampler() override;

    VkSampler getVkSampler() const;

private:
    VulkanDevice& m_device;

private:
    VkSampler m_sampler = VK_NULL_HANDLE;
};

DOWN_CAST(VulkanSampler, Sampler);

// Convert Helper
VkSamplerAddressMode ToVkSamplerAddressMode(AddressMode mode);
AddressMode ToAddressMode(VkSamplerAddressMode mode);
VkFilter ToVkFilter(FilterMode mode);
FilterMode ToFilterMode(VkFilter filter);
VkSamplerMipmapMode ToVkSamplerMipmapMode(MipmapFilterMode mode);
MipmapFilterMode ToMipmapFilterMode(VkSamplerMipmapMode mode);

}; // namespace jipu
