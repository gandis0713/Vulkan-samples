#pragma once

#include "bind_group_layout.h"
#include "jipu/common/cast.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

#include <unordered_map>

namespace jipu
{

struct VulkanBindGroupLayoutInfo
{
    std::vector<BufferBindingLayout> buffers{};
    std::vector<SamplerBindingLayout> samplers{};
    std::vector<TextureBindingLayout> textures{};
    std::vector<StorageTextureBindingLayout> storageTextures{};
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
    std::vector<StorageTextureBindingLayout> getStorageTextureBindingLayouts() const override;

    std::vector<VkDescriptorSetLayoutBinding> getDescriptorSetLayouts() const;

    std::vector<VkDescriptorSetLayoutBinding> getBufferDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getBufferDescriptorSetLayout(uint32_t index) const;

    std::vector<VkDescriptorSetLayoutBinding> getSamplerDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getSamplerDescriptorSetLayout(uint32_t index) const;

    std::vector<VkDescriptorSetLayoutBinding> getTextureDescriptorSetLayouts() const;
    VkDescriptorSetLayoutBinding getTextureDescriptorSetLayout(uint32_t index) const;

    VkDescriptorSetLayout getVkDescriptorSetLayout() const;
    const VulkanBindGroupLayoutInfo& getInfo() const;

private:
    VulkanDevice* m_device = nullptr;
    const BindGroupLayoutDescriptor m_descriptor{};
    const VulkanBindGroupLayoutDescriptor m_vkdescriptor{};
    VulkanBindGroupLayoutInfo m_info{};
};
DOWN_CAST(VulkanBindGroupLayout, BindGroupLayout);

struct VulkanBindGroupLayoutMetaData
{
    VulkanBindGroupLayoutInfo info{};
};
class VulkanBindGroupLayoutCache
{
public:
    VulkanBindGroupLayoutCache() = delete;
    VulkanBindGroupLayoutCache(VulkanDevice* device);
    ~VulkanBindGroupLayoutCache();

public:
    VkDescriptorSetLayout getVkDescriptorSetLayout(const VulkanBindGroupLayoutMetaData& metaData);
    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    struct Functor
    {
        size_t operator()(const VulkanBindGroupLayoutMetaData& metaData) const;
        bool operator()(const VulkanBindGroupLayoutMetaData& lhs, const VulkanBindGroupLayoutMetaData& rhs) const;
    };
    using Cache = std::unordered_map<VulkanBindGroupLayoutMetaData, VkDescriptorSetLayout, Functor, Functor>;
    Cache m_bindGroupLayouts{};
};

// Generate Helper
VulkanBindGroupLayoutDescriptor VULKAN_EXPORT generateVulkanBindGroupLayoutDescriptor(const BindGroupLayoutDescriptor& descriptor);

// Convert Helper
VkDescriptorType ToVkDescriptorType(StorageTextureBindingType type);
VkDescriptorType ToVkDescriptorType(BufferBindingType type, bool dynamicOffset = false);
BufferBindingType ToBufferBindingType(VkDescriptorType type);
VkShaderStageFlags ToVkShaderStageFlags(BindingStageFlags flags);
BindingStageFlags ToBindingStageFlags(VkShaderStageFlags flags);
} // namespace jipu