#pragma once

#include "bind_group_layout.h"
#include "jipu/common/cast.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

struct VulkanBindGroupLayoutDescriptor
{
    const void* next = nullptr;
    VkDescriptorSetLayoutCreateFlags flags = 0u;
    std::vector<VkDescriptorSetLayoutBinding> buffers{};
    std::vector<VkDescriptorSetLayoutBinding> samplers{};
    std::vector<VkDescriptorSetLayoutBinding> textures{};
};

class VulkanDevice;
class VULKAN_EXPORT VulkanBindGroupLayout : public BindGroupLayout
{
public:
    VulkanBindGroupLayout() = delete;
    VulkanBindGroupLayout(VulkanDevice& device, const BindGroupLayoutDescriptor& descriptor);
    VulkanBindGroupLayout(VulkanDevice& device, const VulkanBindGroupLayoutDescriptor& descriptor);
    ~VulkanBindGroupLayout() override;

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
    const VulkanBindGroupLayoutDescriptor m_descriptor{};
};
DOWN_CAST(VulkanBindGroupLayout, BindGroupLayout);

// Generate Helper
VulkanBindGroupLayoutDescriptor VULKAN_EXPORT generateVulkanBindGroupLayoutDescriptor(const BindGroupLayoutDescriptor& descriptor);

// Convert Helper
VkDescriptorType ToVkDescriptorType(BufferBindingType type, bool dynamicOffset = false);
BufferBindingType ToBufferBindingType(VkDescriptorType type);
VkShaderStageFlags ToVkShaderStageFlags(BindingStageFlags flags);
BindingStageFlags ToBindingStageFlags(VkShaderStageFlags flags);
} // namespace jipu