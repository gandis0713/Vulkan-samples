#pragma once

#include "vulkan_api.h"
#include "vulkan_command_recorder.h"

namespace jipu
{

struct VulkanCommandBufferTrackerDescriptor
{
    std::vector<std::pair<CommandBuffer*, VulkanCommandRecordResult>> commandRecordResults{};
};

struct CommandBufferUsageInfo
{
    VkPipelineStageFlags srcStageFlags = 0u;
    VkPipelineStageFlags dstStageFlags = 0u;
};
struct CommandBufferTrackingResult
{
    std::vector<std::pair<CommandBuffer*, CommandBufferUsageInfo>> commandRecordResults{};
};

class VulkanCommandBufferTracker final
{
public:
    VulkanCommandBufferTracker() = default;
    VulkanCommandBufferTracker(const VulkanCommandBufferTrackerDescriptor& descriptor);
    ~VulkanCommandBufferTracker() = default;

public:
};

} // namespace jipu