#pragma once

#include "jipu/common/cast.h"
#include "pipeline_layout.h"
#include "vulkan_api.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_export.h"

namespace jipu
{

struct PipelineLayoutMetaInfo
{
    std::vector<BindGroupLayoutMetaInfo> bindGroupLayoutMetaInfos{};
};

class VulkanDevice;
class VULKAN_EXPORT VulkanPipelineLayout : public PipelineLayout
{
public:
    VulkanPipelineLayout() = delete;
    VulkanPipelineLayout(VulkanDevice* device, const PipelineLayoutDescriptor& descriptor);
    ~VulkanPipelineLayout() override;

public:
    VkPipelineLayout getVkPipelineLayout() const;
    PipelineLayoutMetaInfo getLayoutInfo() const;

private:
    VulkanDevice* m_device = nullptr;

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    const PipelineLayoutDescriptor m_descriptor{};
};
DOWN_CAST(VulkanPipelineLayout, PipelineLayout);

class VulkanPipelineLayoutCache
{
public:
    VulkanPipelineLayoutCache() = delete;
    VulkanPipelineLayoutCache(VulkanDevice* device);
    ~VulkanPipelineLayoutCache();

public:
    VkPipelineLayout getVkPipelineLayout(const PipelineLayoutDescriptor& descriptor);
    VkPipelineLayout getVkPipelineLayout(const PipelineLayoutMetaInfo& layoutInfo);
    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    struct Functor
    {
        size_t operator()(const PipelineLayoutMetaInfo& descriptor) const;
        bool operator()(const PipelineLayoutMetaInfo& lhs, const PipelineLayoutMetaInfo& rhs) const;
    };
    using Cache = std::unordered_map<PipelineLayoutMetaInfo, VkPipelineLayout, Functor, Functor>;
    Cache m_pipelineLayouts{};
};

} // namespace jipu