#pragma once

#include "export.h"

#include "binding_group.h"
#include "jipu/common/cast.h"
#include "vulkan_api.h"
#include "vulkan_binding_group_layout.h"
#include "vulkan_export.h"

namespace jipu
{

class VulkanDevice;
class VULKAN_EXPORT VulkanBindingGroup : public BindingGroup
{
public:
    VulkanBindingGroup() = delete;
    VulkanBindingGroup(VulkanDevice& device, const BindingGroupDescriptor& descriptor);
    ~VulkanBindingGroup() override;

public:
    BindingGroupLayout* getLayout() const override;
    const std::vector<BufferBinding>& getBufferBindings() const override;
    const std::vector<SamplerBinding>& getSmaplerBindings() const override;
    const std::vector<TextureBinding>& getTextureBindings() const override;

public:
    VkDescriptorSet getVkDescriptorSet() const;

private:
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

private:
    VulkanDevice& m_device;
    const BindingGroupDescriptor m_descriptor;

public:
    using Ref = std::reference_wrapper<VulkanBindingGroup>;
};
DOWN_CAST(VulkanBindingGroup, BindingGroup);

} // namespace jipu