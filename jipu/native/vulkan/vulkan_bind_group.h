#pragma once

#include "export.h"

#include "bind_group.h"
#include "jipu/common/cast.h"
#include "vulkan_api.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_export.h"

namespace jipu
{

class VulkanDevice;
class VULKAN_EXPORT VulkanBindGroup : public BindGroup
{
public:
    VulkanBindGroup() = delete;
    VulkanBindGroup(VulkanDevice* device, const BindGroupDescriptor& descriptor);
    ~VulkanBindGroup() override;

public:
    VulkanDevice* getDevice() const;
    const std::vector<BufferBinding>& getBufferBindings() const override;
    const std::vector<SamplerBinding>& getSmaplerBindings() const override;
    const std::vector<TextureBinding>& getTextureBindings() const override;

public: // for information about bind group layout.
    const std::vector<BufferBindingLayout>& getBufferLayouts() const;
    const std::vector<SamplerBindingLayout>& getSamplerLayouts() const;
    const std::vector<TextureBindingLayout>& getTextureLayouts() const;

public:
    VkDescriptorSet getVkDescriptorSet() const;

private:
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

private:
    VulkanDevice* m_device = nullptr;
    const BindGroupDescriptor m_descriptor;

private: // for information about bind group layout. TODO: use cached layout
    BindGroupLayoutDescriptor m_layoutDescriptor{};

public:
    using Ref = std::reference_wrapper<VulkanBindGroup>;
};
DOWN_CAST(VulkanBindGroup, BindGroup);

} // namespace jipu