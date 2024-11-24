#include "vulkan_inflight_objects.h"

#include "vulkan_bind_group.h"
#include "vulkan_bind_group_layout.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_framebuffer.h"
#include "vulkan_pipeline.h"
#include "vulkan_pipeline_layout.h"
#include "vulkan_render_pass.h"
#include "vulkan_sampler.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include <spdlog/spdlog.h>

namespace jipu
{

VulkanInflightObjects::VulkanInflightObjects(VulkanDevice* device)
    : m_device(device)
{
}

VulkanInflightObjects::~VulkanInflightObjects()
{
    clearAll();
}

void VulkanInflightObjects::add(VkFence fence, const std::vector<VulkanSubmit>& submits)
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    auto& inflightObject = m_inflightObjects[fence];

    for (const auto& submit : submits)
    {
        inflightObject.commandBuffers.insert(submit.info.commandBuffers.begin(), submit.info.commandBuffers.end());
        inflightObject.signalSemaphores.insert(submit.info.signalSemaphores.begin(), submit.info.signalSemaphores.end());
        inflightObject.waitSemaphores.insert(submit.info.waitSemaphores.begin(), submit.info.waitSemaphores.end()); // for swapchain

        inflightObject.imageViews.insert(submit.object.imageViews.begin(), submit.object.imageViews.end());
        inflightObject.samplers.insert(submit.object.samplers.begin(), submit.object.samplers.end());
        inflightObject.pipelines.insert(submit.object.pipelines.begin(), submit.object.pipelines.end());
        inflightObject.pipelineLayouts.insert(submit.object.pipelineLayouts.begin(), submit.object.pipelineLayouts.end());
        inflightObject.descriptorSet.insert(submit.object.descriptorSet.begin(), submit.object.descriptorSet.end());
        inflightObject.descriptorSetLayouts.insert(submit.object.descriptorSetLayouts.begin(), submit.object.descriptorSetLayouts.end());
        inflightObject.framebuffers.insert(submit.object.framebuffers.begin(), submit.object.framebuffers.end());
        inflightObject.renderPasses.insert(submit.object.renderPasses.begin(), submit.object.renderPasses.end());

        // for buffer
        {
            for (const auto& bufferResource : submit.object.srcResource.buffers)
            {
                inflightObject.buffers.insert({ bufferResource.first, bufferResource.second });
            }

            for (const auto& bufferResource : submit.object.dstResource.buffers)
            {
                inflightObject.buffers.insert({ bufferResource.first, bufferResource.second });
            }
        }

        // for image
        {
            for (const auto& imageResource : submit.object.srcResource.images)
            {
                inflightObject.images.insert({ imageResource.first, imageResource.second });
            }

            for (const auto& imageResource : submit.object.dstResource.images)
            {
                inflightObject.images.insert({ imageResource.first, imageResource.second });
            }
        }
    }
}

bool VulkanInflightObjects::clear(VkFence fence)
{
    std::optional<VulkanInflightObject> inflightObject{ std::nullopt };

    {
        std::lock_guard<std::mutex> lock(m_objectMutex);
        if (m_inflightObjects.contains(fence))
        {
            inflightObject = m_inflightObjects[fence];
            m_inflightObjects.erase(fence); // erase before calling subscribers.
        }
        else
        {
            spdlog::error("Failed to clear inflight object. The fence is not found.");
        }
    }

    if (inflightObject.has_value() == false)
    {
        std::lock_guard<std::mutex> lock(m_subscribeMutex);
        for (const auto& [_, sub] : m_subs)
        {
            sub(fence, inflightObject.value());
        }

        return true;
    }

    return false;
}

void VulkanInflightObjects::clearAll()
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    m_inflightObjects.clear();
}

void VulkanInflightObjects::subscribe(void* ptr, Subscribe sub)
{
    std::lock_guard<std::mutex> lock(m_subscribeMutex);

    m_subs[ptr] = sub;
}
void VulkanInflightObjects::unsubscribe(void* ptr)
{
    std::lock_guard<std::mutex> lock(m_subscribeMutex);

    if (m_subs.contains(ptr))
        m_subs.erase(ptr);
}

bool VulkanInflightObjects::isInflight(VkCommandBuffer commandBuffer) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.commandBuffers.contains(commandBuffer))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkBuffer buffer) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.buffers.contains(buffer))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkImage image) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.images.contains(image))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkImageView imageView) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.imageViews.contains(imageView))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkSemaphore semaphore) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.signalSemaphores.contains(semaphore) || inflightObject.waitSemaphores.contains(semaphore))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkSampler sampler) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.samplers.contains(sampler))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkPipeline pipeline) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.pipelines.contains(pipeline))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkPipelineLayout pipelineLayout) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.pipelineLayouts.contains(pipelineLayout))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkDescriptorSet descriptorSet) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.descriptorSet.contains(descriptorSet))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkDescriptorSetLayout descriptorSetLayout) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.descriptorSetLayouts.contains(descriptorSetLayout))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkFramebuffer framebuffer) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.framebuffers.contains(framebuffer))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkRenderPass renderPass) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.renderPasses.contains(renderPass))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkFence fence) const
{
    std::lock_guard<std::mutex> lock(m_objectMutex);

    return m_inflightObjects.contains(fence);
}

} // namespace jipu