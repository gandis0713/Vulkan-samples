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
    waitIdle();
}

void VulkanQueue::submit(std::vector<CommandBuffer*> commandBuffers)
{
    // generate Submit context.
    VulkanSubmitContext submitContext = VulkanSubmitContext::create(m_device, commandBuffers);

    // submit
    auto submits = submitContext.getSubmits();
    auto future = m_submitter->submitAsync(submits);

    // set present semaphores.
    {
        std::optional<VulkanSubmit::Info> presentSubmitInfo{ std::nullopt };
        for (const auto& submit : submits)
        {
            if (submit.info.type == SubmitType::kPresent)
            {
                presentSubmitInfo = submit.info;
                break;
            }
        }

        if (presentSubmitInfo.has_value())
        {
            auto& submitInfo = presentSubmitInfo.value();
            auto& index = submitInfo.swapchainIndex;

            if (m_presentTasks.contains(index))
            {
                m_presentTasks[index].get(); // wait previous present task.
                m_presentTasks.erase(index);
            }

            m_presentTasks[index] = std::move(future);
            m_presentSignalSemaphores[index].insert(m_presentSignalSemaphores[index].end(), submitInfo.signalSemaphores.begin(), submitInfo.signalSemaphores.end());
        }
        else
        {
            m_notPresentTasks.push(std::move(future));
        }
    }
}

void VulkanQueue::waitIdle()
{
    m_submitter->waitIdle();

    while (!m_notPresentTasks.empty())
    {
        auto task = std::move(m_notPresentTasks.front());
        m_notPresentTasks.pop();

        task.get();
    }

    for (auto& [_, task] : m_presentTasks)
    {
        task.get();
    }
    m_presentTasks.clear();
}

void VulkanQueue::present(VulkanPresentInfo presentInfo)
{
    for (auto imageIndex : presentInfo.imageIndices)
    {
        if (m_presentSignalSemaphores.contains(imageIndex))
        {
            presentInfo.waitSemaphores.insert(presentInfo.waitSemaphores.end(),
                                              m_presentSignalSemaphores[imageIndex].begin(),
                                              m_presentSignalSemaphores[imageIndex].end());

            m_presentSignalSemaphores.erase(imageIndex);
        }
    }

    m_submitter->present(presentInfo);
}

} // namespace jipu
