#include "vulkan_queue.h"

#include "vulkan_device.h"
#include "vulkan_physical_device.h"
#include "vulkan_submit_context.h"
#include "vulkan_swapchain.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

VulkanQueue::VulkanQueue(VulkanDevice& device, const QueueDescriptor& descriptor) noexcept(false)
    : m_device(device)
    , m_submitter(std::make_unique<VulkanSubmitter>(&device))
{
}

VulkanQueue::~VulkanQueue()
{
}

void VulkanQueue::submit(std::vector<CommandBuffer*> commandBuffers)
{
    // record VulkanCommandBuffer to VkCommandBuffer.
    std::vector<VulkanCommandRecordResult> commandRecordResults = recordCommands(commandBuffers);

    // generate Submit context.
    VulkanSubmitContext submitContext = VulkanSubmitContext::create(&m_device, commandRecordResults);

    // set inflight vulkan objects.
    m_device.getInflightContext();

    // submit
    auto submitInfos = submitContext.getSubmitInfos();

    auto isPresentSubmit = [&](const auto& submitInfos) -> bool {
        for (const auto& submitInfo : submitInfos)
        {
            if (submitInfo.type == SubmitType::kPresent)
                return true;
        }
        return false;
    }(submitInfos);

    if (isPresentSubmit)
    {
        auto& presentSubmitInfos = m_presentSubmitInfos;
        presentSubmitInfos.insert(presentSubmitInfos.end(), submitInfos.begin(), submitInfos.end());
    }
    else
    {
        auto future = m_submitter->submitAsync(submitInfos);
        future.get();
    }
}

void VulkanQueue::present(VulkanPresentInfo presentInfo)
{
    auto future = m_submitter->presentAsync(m_presentSubmitInfos, presentInfo);
    future.get();

    m_presentSubmitInfos = {};
}

std::vector<VulkanCommandRecordResult> VulkanQueue::recordCommands(std::vector<CommandBuffer*> commandBuffers)
{
    std::vector<VulkanCommandRecordResult> commandRecordResult{};

    for (auto& commandBuffer : commandBuffers)
    {
        auto vulkanCommandBuffer = downcast(commandBuffer);
        commandRecordResult.push_back(vulkanCommandBuffer->recordToVkCommandBuffer());
    }

    return commandRecordResult;
}

} // namespace jipu
