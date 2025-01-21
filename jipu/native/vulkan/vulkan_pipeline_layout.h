#pragma once

#include "jipu/common/cast.h"
#include "pipeline_layout.h"
#include "vulkan_api.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_export.h"

namespace jipu
{

struct VulkanPipelineLayoutInfo
{
    std::vector<VulkanBindGroupLayoutInfo> bindGroupLayoutInfos{};
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
    const VulkanPipelineLayoutInfo& getInfo() const;

private:
    VulkanDevice* m_device = nullptr;

private:
    const PipelineLayoutDescriptor m_descriptor{};
    VulkanPipelineLayoutInfo m_info{};
};
DOWN_CAST(VulkanPipelineLayout, PipelineLayout);

struct VulkanPipelineLayoutMetaData
{
    VulkanPipelineLayoutInfo info{};
};
class VulkanPipelineLayoutCache
{
public:
    VulkanPipelineLayoutCache() = delete;
    VulkanPipelineLayoutCache(VulkanDevice* device);
    ~VulkanPipelineLayoutCache();

public:
    VkPipelineLayout getVkPipelineLayout(const VulkanPipelineLayoutMetaData& metaData);
    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    struct Functor
    {
        size_t operator()(const VulkanPipelineLayoutMetaData& metaData) const;
        bool operator()(const VulkanPipelineLayoutMetaData& lhs, const VulkanPipelineLayoutMetaData& rhs) const;
    };
    using Cache = std::unordered_map<VulkanPipelineLayoutMetaData, VkPipelineLayout, Functor, Functor>;
    Cache m_pipelineLayouts{};
};

} // namespace jipu