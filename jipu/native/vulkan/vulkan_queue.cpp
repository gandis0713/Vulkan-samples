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

    // submit
    auto submits = submitContext.getSubmits();

    std::vector<VulkanSubmit::Info> notPresentSubmitInfos{};
    std::vector<VulkanSubmit::Info> presentSubmitInfos{};
    std::vector<VulkanSubmit> notPresnetSubmits{};
    std::vector<VulkanSubmit> presentSubmits{};
    for (const auto& submit : submits)
    {
        switch (submit.info.type)
        {
        case SubmitType::kCompute:
        case SubmitType::kRender:
        case SubmitType::kTransfer:
            notPresentSubmitInfos.push_back(submit.info);
            notPresnetSubmits.push_back(submit);
            break;
        case SubmitType::kPresent:
            presentSubmitInfos.push_back(submit.info);
            presentSubmits.push_back(submit);
            break;
        case SubmitType::kNone:
        default:
            spdlog::error("Failed to collect submit. The kNone submit type is not supported.");
            break;
        }
    }

    if (!notPresentSubmitInfos.empty())
    {
        auto fence = m_device.getFencePool()->create();
        m_device.getInflightObjects()->add(fence, notPresnetSubmits);

        auto future = m_submitter->submitAsync(fence, notPresentSubmitInfos);
        future.get();
    }

    if (!presentSubmitInfos.empty())
    {
        if (m_presentSubmitInfos.first == VK_NULL_HANDLE)
        {
            m_presentSubmitInfos.first = m_device.getFencePool()->create();
        }

        auto fence = m_presentSubmitInfos.first;
        m_device.getInflightObjects()->add(fence, presentSubmits);

        m_presentSubmitInfos.second.insert(m_presentSubmitInfos.second.end(),
                                           presentSubmitInfos.begin(),
                                           presentSubmitInfos.end());
    }
}

void VulkanQueue::present(VulkanPresentInfo presentInfo)
{
    auto future = m_submitter->presentAsync(m_presentSubmitInfos.first, m_presentSubmitInfos.second, presentInfo);
    future.get();

    m_presentSubmitInfos = { VK_NULL_HANDLE, {} };
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
