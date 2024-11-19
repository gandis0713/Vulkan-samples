#pragma once

#include "jipu/common/cast.h"
#include "pipeline_layout.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

class VulkanDevice;
class VULKAN_EXPORT VulkanPipelineLayout : public PipelineLayout
{
public:
    VulkanPipelineLayout() = delete;
    VulkanPipelineLayout(VulkanDevice* device, const PipelineLayoutDescriptor& descriptor);
    ~VulkanPipelineLayout() override;

public:
    VkPipelineLayout getVkPipelineLayout() const;

private:
    VulkanDevice* m_device = nullptr;

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};

DOWN_CAST(VulkanPipelineLayout, PipelineLayout);

} // namespace jipu