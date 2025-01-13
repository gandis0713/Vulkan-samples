#pragma once

#include "bind_group_layout.h"
#include "jipu/common/cast.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include <unordered_map>

namespace jipu
{

struct BindGroupLayoutInfo
{
    std::vector<BufferBindingLayout> buffers = {};
    std::vector<SamplerBindingLayout> samplers = {};
    std::vector<TextureBindingLayout> textures = {};
};

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
    VulkanBindGroupLayout(VulkanDevice* device, const BindGroupLayoutDescriptor& descriptor);
    ~VulkanBindGroupLayout() override;

    std::vector<BufferBindingLayout> getBufferBindingLayouts() const override;
    std::vector<SamplerBindingLayout> getSamplerBindingLayouts() const override;
    std::vector<TextureBindingLayout> getTextureBindingLayouts() const override;

    std::vector<VkDescriptorSetLayoutBinding> getDescriptorSetLayouts() const;

    std::vector<VkDescriptorSetLayoutBinding> getBufferDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getBufferDescriptorSetLayout(uint32_t index) const;

    std::vector<VkDescriptorSetLayoutBinding> getSamplerDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getSamplerDescriptorSetLayout(uint32_t index) const;

    std::vector<VkDescriptorSetLayoutBinding> getTextureDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getTextureDescriptorSetLayout(uint32_t index) const;

    VkDescriptorSetLayout getVkDescriptorSetLayout() const;
    BindGroupLayoutInfo getLayoutInfo() const;

private:
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

private:
    VulkanDevice* m_device = nullptr;
    const VulkanBindGroupLayoutDescriptor m_descriptor{};
};
DOWN_CAST(VulkanBindGroupLayout, BindGroupLayout);

class VulkanBindGroupLayoutCache
{
public:
    VulkanBindGroupLayoutCache() = delete;
    VulkanBindGroupLayoutCache(VulkanDevice* device);
    ~VulkanBindGroupLayoutCache();

public:
    VkDescriptorSetLayout getVkDescriptorSetLayout(const BindGroupLayoutDescriptor& descriptor);
    VkDescriptorSetLayout getVkDescriptorSetLayout(const BindGroupLayoutInfo& layoutInfo);
    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    struct Functor
    {
        size_t operator()(const BindGroupLayoutInfo& descriptor) const;
        bool operator()(const BindGroupLayoutInfo& lhs, const BindGroupLayoutInfo& rhs) const;
    };
    using Cache = std::unordered_map<BindGroupLayoutInfo, VkDescriptorSetLayout, Functor, Functor>;
    Cache m_bindGroupLayouts{};
};

// Generate Helper
VulkanBindGroupLayoutDescriptor VULKAN_EXPORT generateVulkanBindGroupLayoutDescriptor(const BindGroupLayoutDescriptor& descriptor);

// Convert Helper
VkDescriptorType ToVkDescriptorType(BufferBindingType type, bool dynamicOffset = false);
BufferBindingType ToBufferBindingType(VkDescriptorType type);
VkShaderStageFlags ToVkShaderStageFlags(BindingStageFlags flags);
BindingStageFlags ToBindingStageFlags(VkShaderStageFlags flags);
} // namespace jipu