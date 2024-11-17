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

namespace jipu
{

VulkanInflightObjects::VulkanInflightObjects(VulkanDevice* device)
    : m_device(device)
{
}

VulkanInflightObjects::~VulkanInflightObjects()
{
    m_inflightObjects.clear();
}

void VulkanInflightObjects::add(VkFence fence, const std::vector<VulkanSubmit>& submits)
{
    auto& inflightObject = m_inflightObjects[fence];

    for (const auto& submit : submits)
    {
        inflightObject.commandBuffers.insert(submit.info.commandBuffers.begin(), submit.info.commandBuffers.end());
        inflightObject.signalSemaphores.insert(submit.info.signalSemaphores.begin(), submit.info.signalSemaphores.end());
        // do not need to wait for semaphores.

        inflightObject.imageViews.insert(submit.object.imageViews.begin(), submit.object.imageViews.end());
        inflightObject.samplers.insert(submit.object.samplers.begin(), submit.object.samplers.end());
        inflightObject.pipelines.insert(submit.object.pipelines.begin(), submit.object.pipelines.end());
        inflightObject.pipelineLayouts.insert(submit.object.pipelineLayouts.begin(), submit.object.pipelineLayouts.end());
        inflightObject.descriptorSet.insert(submit.object.descriptorSet.begin(), submit.object.descriptorSet.end());
        inflightObject.descriptorSetLayouts.insert(submit.object.descriptorSetLayouts.begin(), submit.object.descriptorSetLayouts.end());
        inflightObject.framebuffers.insert(submit.object.framebuffers.begin(), submit.object.framebuffers.end());
        inflightObject.renderPasses.insert(submit.object.renderPasses.begin(), submit.object.renderPasses.end());

        inflightObject.buffers.insert(submit.object.srcResource.buffers.begin(), submit.object.srcResource.buffers.end());
        inflightObject.buffers.insert(submit.object.dstResource.buffers.begin(), submit.object.dstResource.buffers.end());

        inflightObject.images.insert(submit.object.srcResource.images.begin(), submit.object.srcResource.images.end());
        inflightObject.images.insert(submit.object.dstResource.images.begin(), submit.object.dstResource.images.end());
    }
}

bool VulkanInflightObjects::clear(VkFence fence)
{
    if (m_inflightObjects.contains(fence))
    {
        auto inflightObject = m_inflightObjects[fence];
        m_inflightObjects.erase(fence); // erase before calling subscribers.

        for (const auto& [ptr, sub] : m_subs)
        {
            sub(fence, inflightObject);
        }

        return true;
    }

    return false;
}

void VulkanInflightObjects::clearAll()
{
}

void VulkanInflightObjects::subscribe(void* ptr, Subscribe sub)
{
    m_subs[ptr] = sub;
}
void VulkanInflightObjects::unsubscribe(void* ptr)
{
    if (m_subs.contains(ptr))
        m_subs.erase(ptr);
}

bool VulkanInflightObjects::isInflight(VkCommandBuffer commandBuffer) const
{
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
    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.signalSemaphores.contains(semaphore))
        {
            return true;
        }
    }

    return false;
}

bool VulkanInflightObjects::isInflight(VkSampler sampler) const
{
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
    for (const auto& [_, inflightObject] : m_inflightObjects)
    {
        if (inflightObject.renderPasses.contains(renderPass))
        {
            return true;
        }
    }

    return false;
}

} // namespace jipu