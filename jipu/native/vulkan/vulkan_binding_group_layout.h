#pragma once

#include "binding_group_layout.h"
#include "jipu/common/cast.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

struct VulkanBindingGroupLayoutDescriptor
{
    const void* next = nullptr;
    VkDescriptorSetLayoutCreateFlags flags = 0u;
    std::vector<VkDescriptorSetLayoutBinding> buffers{};
    std::vector<VkDescriptorSetLayoutBinding> samplers{};
    std::vector<VkDescriptorSetLayoutBinding> textures{};
};

class VulkanDevice;
class VULKAN_EXPORT VulkanBindingGroupLayout : public BindingGroupLayout
{
public:
    VulkanBindingGroupLayout() = delete;
    VulkanBindingGroupLayout(VulkanDevice& device, const BindingGroupLayoutDescriptor& descriptor);
    VulkanBindingGroupLayout(VulkanDevice& device, const VulkanBindingGroupLayoutDescriptor& descriptor);
    ~VulkanBindingGroupLayout() override;

    std::vector<BufferBindingLayout> getBufferBindingLayouts() const override;
    std::vector<SamplerBindingLayout> getSamplerBindingLayouts() const override;
    std::vector<TextureBindingLayout> getTextureBindingLayouts() const override;

    std::vector<VkDescriptorSetLayoutBinding> getBufferDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getBufferDescriptorSetLayout(uint32_t index) const;

    std::vector<VkDescriptorSetLayoutBinding> getSamplerDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getSamplerDescriptorSetLayout(uint32_t index) const;

    std::vector<VkDescriptorSetLayoutBinding> getTextureDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getTextureDescriptorSetLayout(uint32_t index) const;

    VkDescriptorSetLayout getVkDescriptorSetLayout() const;

private:
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

private:
    VulkanDevice& m_device;
    const VulkanBindingGroupLayoutDescriptor m_descriptor{};
};
DOWN_CAST(VulkanBindingGroupLayout, BindingGroupLayout);

// Generate Helper
VulkanBindingGroupLayoutDescriptor VULKAN_EXPORT generateVulkanBindingGroupLayoutDescriptor(const BindingGroupLayoutDescriptor& descriptor);

// Convert Helper
VkDescriptorType ToVkDescriptorType(BufferBindingType type, bool dynamicOffset = false);
BufferBindingType ToBufferBindingType(VkDescriptorType type);
VkShaderStageFlags ToVkShaderStageFlags(BindingStageFlags flags);
BindingStageFlags ToBindingStageFlags(VkShaderStageFlags flags);
} // namespace jipu