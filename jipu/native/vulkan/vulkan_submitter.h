#pragma once

#include "jipu/common/thread_pool.h"
#include "vulkan_submit_context.h"
#include "vulkan_swapchain.h"

#include <future>

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
class VulkanSubmitter
{
public:
    VulkanSubmitter() = delete;
    VulkanSubmitter(VulkanDevice* device);
    ~VulkanSubmitter();

public:
    void submit(VkFence fence, const std::vector<VulkanSubmit::Info>& submits);
    void present(VkFence fence, std::vector<VulkanSubmit::Info> submitInfos, VulkanPresentInfo presentInfo);
    std::future<void> submitAsync(VkFence fence, const std::vector<VulkanSubmit::Info>& submits);
    std::future<void> presentAsync(VkFence fence, std::vector<VulkanSubmit::Info> submitInfos, VulkanPresentInfo presentInfo);

private:
    struct QueueGroup
    {
        VulkanQueueFlags flags;
        std::vector<VkQueue> queues{};
    };

private:
    VkQueue getVkQueue(VulkanQueueFlags flags = VulkanQueueFlagBits::kAll) const;

private:
    VulkanDevice* m_device = nullptr;
    std::vector<QueueGroup> m_queueGroups{};

    ThreadPool m_threadPool{ 10 };
};

// Convert Helper
VkQueueFlags ToVkQueueFlags(VulkanQueueFlags flags);
VulkanQueueFlags ToQueueFlags(VkQueueFlags vkflags);

} // namespace jipu