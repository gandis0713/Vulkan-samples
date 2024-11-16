#pragma once

#include "jipu/common/cast.h"
#include "queue.h"
#include "vulkan_api.h"
#include "vulkan_command_recorder.h"
#include "vulkan_export.h"
#include "vulkan_inflight_context.h"
#include "vulkan_submit_context.h"
#include "vulkan_submitter.h"
#include "vulkan_swapchain.h"

#include <unordered_map>

namespace jipu
{

class VulkanDevice;
class VULKAN_EXPORT VulkanQueue : public Queue
{
public:
    VulkanQueue() = delete;
    VulkanQueue(VulkanDevice& device, const QueueDescriptor& descriptor) noexcept(false);
    ~VulkanQueue() override;

public:
    void submit(std::vector<CommandBuffer*> commandBuffers) override;

public:
    void present(VulkanPresentInfo presentInfo);

private:
    std::vector<VulkanCommandRecordResult> recordCommands(std::vector<CommandBuffer*> commandBuffers);

private:
    VulkanDevice& m_device;
    std::unique_ptr<VulkanSubmitter> m_submitter = nullptr;

private:
    std::vector<VulkanSubmit::Info> m_presentSubmitInfos{};
};

DOWN_CAST(VulkanQueue, Queue);

} // namespace jipu