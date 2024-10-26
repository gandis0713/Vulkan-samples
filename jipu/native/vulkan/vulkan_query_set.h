#pragma once

#include "jipu/common/cast.h"
#include "query_set.h"
#include "vulkan_api.h"
#include "vulkan_export.h"

namespace jipu
{

class VulkanDevice;
class VULKAN_EXPORT VulkanQuerySet : public QuerySet
{
public:
    VulkanQuerySet() = delete;
    VulkanQuerySet(VulkanDevice& device, const QuerySetDescriptor& descriptor);
    ~VulkanQuerySet() override;

    QueryType getType() const override;
    uint32_t getCount() const override;

public:
    VkQueryPool getVkQueryPool() const;

private:
    VulkanDevice& m_device;
    const QuerySetDescriptor m_descriptor;

private:
    VkQueryPool m_queryPool = VK_NULL_HANDLE;
};
DOWN_CAST(VulkanQuerySet, QuerySet);

// Convert Helper
VkQueryType ToVkQueryType(QueryType type);

} // namespace jipu