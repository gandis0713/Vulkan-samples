#pragma once

#include "jipu/common/cast.h"
#include "queue.h"
#include "vulkan_api.h"
#include "vulkan_command_recorder.h"
#include "vulkan_export.h"
#include "vulkan_inflight_context.h"
#include "vulkan_swapchain.h"

#include <unordered_map>

namespace jipu
{

struct VulkanQueueFlagBits
{
    static constexpr uint8_t kUndefined = 1 << 0; // 0x00000000
    static constexpr uint8_t kGraphics = 1 << 1;  // 0x00000001
    static constexpr uint8_t kCompute = 1 << 2;   // 0x00000002
    static constexpr uint8_t kTransfer = 1 << 3;  // 0x00000004
    static constexpr uint8_t kAll = kGraphics | kCompute | kTransfer;
};
using VulkanQueueFlags = uint8_t;

class VulkanDevice;
class VULKAN_EXPORT VulkanQueue : public Queue
{
public:
    VulkanQueue() = delete;
    VulkanQueue(VulkanDevice& device, const QueueDescriptor& descriptor) noexcept(false);
    ~VulkanQueue() override;

    void submit(std::vector<CommandBuffer*> commandBuffers) override;

public:
    void present(VulkanPresentInfo presentInfo);

private:
    VkQueue getVkQueue(uint32_t index = 0) const;
    VkQueue getVkQueue(VulkanQueueFlags flags = VulkanQueueFlagBits::kAll) const;

private:
    VulkanDevice& m_device;

private:
    VkFence m_fence = VK_NULL_HANDLE;

    std::vector<std::pair<VkQueue, VulkanQueueFlags>> m_queues{};

private:
    std::vector<VulkanSubmit::Info> m_presentSubmitInfos{};

    std::vector<VulkanCommandRecordResult> recordCommands(std::vector<CommandBuffer*> commandBuffers);
    void submit(const std::vector<VulkanSubmit::Info>& submitInfos);
};

DOWN_CAST(VulkanQueue, Queue);

// Convert Helper
VkQueueFlags ToVkQueueFlags(VulkanQueueFlags flags);
VulkanQueueFlags ToQueueFlags(VkQueueFlags vkflags);

} // namespace jipu