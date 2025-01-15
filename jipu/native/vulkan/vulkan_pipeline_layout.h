#pragma once

#include "jipu/common/cast.h"
#include "pipeline_layout.h"
#include "vulkan_api.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_export.h"

namespace jipu
{

struct PipelineLayoutInfo
{
    std::vector<BindGroupLayoutMetaData> bindGroupLayoutMetaDatas{};
};

struct PipelineLayoutMetaData
{
    PipelineLayoutInfo info{};
    size_t hash = 0;
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
    const PipelineLayoutMetaData& getMetaData() const;

private:
    VulkanDevice* m_device = nullptr;

private:
    const PipelineLayoutDescriptor m_descriptor{};
    PipelineLayoutMetaData m_layoutMetaData{};
};
DOWN_CAST(VulkanPipelineLayout, PipelineLayout);

class VulkanPipelineLayoutCache
{
public:
    VulkanPipelineLayoutCache() = delete;
    VulkanPipelineLayoutCache(VulkanDevice* device);
    ~VulkanPipelineLayoutCache();

public:
    VkPipelineLayout getVkPipelineLayout(const PipelineLayoutMetaData& layoutMetaData);
    void clear();

private:
    VulkanDevice* m_device = nullptr;

private:
    // struct Functor
    // {
    //     size_t operator()(const PipelineLayoutMetaData& descriptor) const;
    //     bool operator()(const PipelineLayoutMetaData& lhs, const PipelineLayoutMetaData& rhs) const;
    // };
    using Cache = std::unordered_map<size_t, VkPipelineLayout>;
    Cache m_pipelineLayouts{};
};

} // namespace jipu