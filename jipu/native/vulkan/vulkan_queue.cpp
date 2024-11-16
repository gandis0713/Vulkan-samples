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

    std::vector<VulkanSubmit::Info> immediateSubmitInfos{};
    std::vector<VulkanSubmit::Info> presentSubmitInfos{};
    for (const auto& submitInfo : submitInfos)
    {
        switch (submitInfo.type)
        {
        case SubmitType::kCompute:
        case SubmitType::kRender:
        case SubmitType::kTransfer:
            immediateSubmitInfos.push_back(submitInfo);
            break;
        case SubmitType::kPresent:
            presentSubmitInfos.push_back(submitInfo);
            break;
        case SubmitType::kNone:
        default:
            // log error
            break;
        }
    }

    if (!immediateSubmitInfos.empty())
    {
        auto future = m_submitter->submitAsync(immediateSubmitInfos);
        future.get();
    }

    if (!presentSubmitInfos.empty())
    {
        m_presentSubmitInfos.insert(m_presentSubmitInfos.end(),
                                    presentSubmitInfos.begin(),
                                    presentSubmitInfos.end());
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
