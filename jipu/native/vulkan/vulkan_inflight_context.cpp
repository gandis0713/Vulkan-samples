#include "vulkan_inflight_context.h"

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

VulkanInflightContext::VulkanInflightContext(VulkanDevice* device)
    : m_device(device)
{
}

VulkanInflightContext::~VulkanInflightContext()
{
    m_inflightObjects.clear();
}

void VulkanInflightContext::add(VkFence fence, const VulkanSubmitContext& submitContext)
{
    auto& inflightObject = m_inflightObjects[fence];

    for (const auto& submit : submitContext.getSubmits())
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

bool VulkanInflightContext::clear(VkFence fence)
{
    if (m_inflightObjects.contains(fence))
    {
        m_inflightObjects.erase(fence);
        return true;
    }

    return false;
}

void VulkanInflightContext::clearAll()
{
}

} // namespace jipu