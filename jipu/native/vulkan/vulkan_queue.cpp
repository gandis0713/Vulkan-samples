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

VulkanQueue::VulkanQueue(VulkanDevice* device, const QueueDescriptor& descriptor) noexcept(false)
    : m_device(device)
    , m_submitter(std::make_unique<VulkanSubmitter>(device))
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
    VulkanSubmitContext submitContext = VulkanSubmitContext::create(m_device, commandRecordResults);

    // submit
    auto submits = submitContext.getSubmits();
    auto submitInfos = submitContext.getSubmitInfos();

    auto fence = m_device->getFencePool()->create();
    m_device->getInflightObjects()->add(fence, submits);

    auto future = m_submitter->submitAsync(fence, submitInfos);
    m_device->getDeleter()->safeDestroy(fence); // delete fence after submit.

    // set present semaphores.
    {
        std::vector<VulkanSubmit::Info> presentSubmitInfos{};
        for (const auto& submit : submits)
        {
            switch (submit.info.type)
            {
            case SubmitType::kCompute:
            case SubmitType::kRender:
            case SubmitType::kTransfer:
                break;
            case SubmitType::kPresent:
                presentSubmitInfos.push_back(submit.info);
                break;
            case SubmitType::kNone:
            default:
                spdlog::error("Failed to collect submit. The kNone submit type is not supported.");
                break;
            }
        }

        for (auto& presentSubmitInfo : presentSubmitInfos)
        {
            auto& index = presentSubmitInfo.swapchainIndex;

            if (m_presentTasks.contains(index))
                m_presentTasks[index].get(); // wait previous present task.

            m_presentTasks[index] = std::move(future);
            m_presentSemaphores[index] = presentSubmitInfo.signalSemaphores;
        }
    }

    // future.get();
}

void VulkanQueue::present(VulkanPresentInfo presentInfo)
{
    for (auto imageIndex : presentInfo.imageIndices)
    {
        if (m_presentSemaphores.contains(imageIndex))
        {
            presentInfo.waitSemaphores.insert(presentInfo.waitSemaphores.end(),
                                              m_presentSemaphores[imageIndex].begin(),
                                              m_presentSemaphores[imageIndex].end());

            m_presentSemaphores.erase(imageIndex);
        }
    }

    m_submitter->present(presentInfo);
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
