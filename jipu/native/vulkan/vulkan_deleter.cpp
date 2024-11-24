#include "vulkan_deleter.h"

#include "vulkan_device.h"

#include <spdlog/spdlog.h>

namespace jipu
{

std::unique_ptr<VulkanDeleter> VulkanDeleter::create(VulkanDevice* device)
{
    auto vulkanDeleter = std::unique_ptr<VulkanDeleter>(new VulkanDeleter(device));

    return vulkanDeleter;
}

VulkanDeleter::VulkanDeleter(VulkanDevice* device)
    : m_device(device)
{
    m_device->getInflightObjects()->subscribe(this, [&](VkFence fence, VulkanInflightObject object) {
        for (auto commandBuffer : object.commandBuffers)
        {
            if (m_commandBuffers.contains(commandBuffer))
            {
                safeDestroy(commandBuffer);
            }
        }

        for (auto [buffer, memory] : object.buffers)
        {
            if (m_buffers.contains(buffer))
            {
                safeDestroy(buffer, memory);
            }
        }

        for (auto [image, memory] : object.images)
        {
            if (m_images.contains(image))
            {
                safeDestroy(image, memory);
            }
        }

        for (auto imageView : object.imageViews)
        {
            if (m_imageViews.contains(imageView))
            {
                safeDestroy(imageView);
            }
        }

        for (auto semaphore : object.signalSemaphores)
        {
            if (m_semaphores.contains(semaphore))
            {
                safeDestroy(semaphore);
            }
        }

        for (auto sampler : object.samplers)
        {
            if (m_samplers.contains(sampler))
            {
                safeDestroy(sampler);
            }
        }

        for (auto pipeline : object.pipelines)
        {
            if (m_pipelines.contains(pipeline))
            {
                safeDestroy(pipeline);
            }
        }

        for (auto pipelineLayout : object.pipelineLayouts)
        {
            if (m_pipelineLayouts.contains(pipelineLayout))
            {
                safeDestroy(pipelineLayout);
            }
        }

        for (auto descriptorSet : object.descriptorSet)
        {
            if (m_descriptorSets.contains(descriptorSet))
            {
                safeDestroy(descriptorSet);
            }
        }

        for (auto descriptorSetLayout : object.descriptorSetLayouts)
        {
            if (m_descriptorSetLayouts.contains(descriptorSetLayout))
            {
                safeDestroy(descriptorSetLayout);
            }
        }

        for (auto framebuffer : object.framebuffers)
        {
            if (m_framebuffers.contains(framebuffer))
            {
                safeDestroy(framebuffer);
            }
        }

        for (auto renderPass : object.renderPasses)
        {
            if (m_renderPasses.contains(renderPass))
            {
                safeDestroy(renderPass);
            }
        }

        if (m_fences.contains(fence))
        {
            safeDestroy(fence);
        }
    });
}

VulkanDeleter::~VulkanDeleter()
{
    m_device->getInflightObjects()->unsubscribe(this);

    for (auto [buffer, memory] : m_buffers)
    {
        m_device->getResourceAllocator().destroyBufferResource({ buffer, memory });
    }

    for (auto [image, memory] : m_images)
    {
        m_device->getResourceAllocator().destroyTextureResource({ image, memory });
    }

    for (auto commandBuffer : m_commandBuffers)
    {
        m_device->getCommandPool()->release(commandBuffer);
    }

    for (auto imageView : m_imageViews)
    {
        m_device->vkAPI.DestroyImageView(m_device->getVkDevice(), imageView, nullptr);
    }

    for (auto semaphore : m_semaphores)
    {
        m_device->getSemaphorePool()->release(semaphore);
    }

    for (auto sampler : m_samplers)
    {
        m_device->vkAPI.DestroySampler(m_device->getVkDevice(), sampler, nullptr);
    }

    for (auto pipeline : m_pipelines)
    {
        m_device->vkAPI.DestroyPipeline(m_device->getVkDevice(), pipeline, nullptr);
    }

    for (auto pipelineLayout : m_pipelineLayouts)
    {
        m_device->vkAPI.DestroyPipelineLayout(m_device->getVkDevice(), pipelineLayout, nullptr);
    }

    for (auto descriptorSet : m_descriptorSets)
    {
        m_device->vkAPI.FreeDescriptorSets(m_device->getVkDevice(), m_device->getVkDescriptorPool(), 1, &descriptorSet);
    }

    for (auto descriptorSetLayout : m_descriptorSetLayouts)
    {
        m_device->vkAPI.DestroyDescriptorSetLayout(m_device->getVkDevice(), descriptorSetLayout, nullptr);
    }

    for (auto framebuffer : m_framebuffers)
    {
        m_device->vkAPI.DestroyFramebuffer(m_device->getVkDevice(), framebuffer, nullptr);
    }

    for (auto renderPass : m_renderPasses)
    {
        m_device->vkAPI.DestroyRenderPass(m_device->getVkDevice(), renderPass, nullptr);
    }

    for (auto fence : m_fences)
    {
        m_device->getFencePool()->release(fence);
    }
}

void VulkanDeleter::safeDestroy(VkBuffer buffer, VulkanMemory memory)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(buffer))
    {
        m_buffers.insert({ buffer, memory });
    }
    else
    {
        m_buffers.erase(buffer);
        destroy(buffer, memory);
    }
}

void VulkanDeleter::safeDestroy(VkImage image, VulkanMemory memory)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(image))
    {
        m_images.insert({ image, memory });
    }
    else
    {
        m_images.erase(image);
        destroy(image, memory);
    }
}

void VulkanDeleter::safeDestroy(VkCommandBuffer commandBuffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(commandBuffer))
    {
        m_commandBuffers.insert(commandBuffer);
    }
    else
    {
        m_commandBuffers.erase(commandBuffer);
        destroy(commandBuffer);
    }
}

void VulkanDeleter::safeDestroy(VkImageView imageView)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(imageView))
    {
        m_imageViews.insert(imageView);
    }
    else
    {
        m_imageViews.erase(imageView);
        destroy(imageView);
    }
}
void VulkanDeleter::safeDestroy(VkSemaphore semaphore)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(semaphore))
    {
        m_semaphores.insert(semaphore);
    }
    else
    {
        m_semaphores.erase(semaphore);
        destroy(semaphore);
    }
}

void VulkanDeleter::safeDestroy(VkSampler sampler)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(sampler))
    {
        m_samplers.insert(sampler);
    }
    else
    {
        m_samplers.erase(sampler);
        destroy(sampler);
    }
}
void VulkanDeleter::safeDestroy(VkPipeline pipeline)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(pipeline))
    {
        m_pipelines.insert(pipeline);
    }
    else
    {
        m_pipelines.erase(pipeline);
        destroy(pipeline);
    }
}

void VulkanDeleter::safeDestroy(VkPipelineLayout pipelineLayout)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(pipelineLayout))
    {
        m_pipelineLayouts.insert(pipelineLayout);
    }
    else
    {
        m_pipelineLayouts.erase(pipelineLayout);
        destroy(pipelineLayout);
    }
}

void VulkanDeleter::safeDestroy(VkDescriptorSet descriptorSet)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(descriptorSet))
    {
        m_descriptorSets.insert(descriptorSet);
    }
    else
    {
        m_descriptorSets.erase(descriptorSet);
        destroy(descriptorSet);
    }
}

void VulkanDeleter::safeDestroy(VkDescriptorSetLayout descriptorSetLayout)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(descriptorSetLayout))
    {
        m_descriptorSetLayouts.insert(descriptorSetLayout);
    }
    else
    {
        m_descriptorSetLayouts.erase(descriptorSetLayout);
        destroy(descriptorSetLayout);
    }
}

void VulkanDeleter::safeDestroy(VkFramebuffer framebuffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(framebuffer))
    {
        m_framebuffers.insert(framebuffer);
    }
    else
    {
        m_framebuffers.erase(framebuffer);
        destroy(framebuffer);
    }
}

void VulkanDeleter::safeDestroy(VkRenderPass renderPass)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(renderPass))
    {
        m_renderPasses.insert(renderPass);
    }
    else
    {
        m_renderPasses.erase(renderPass);
        destroy(renderPass);
    }
}

void VulkanDeleter::safeDestroy(VkFence fence)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_device->getInflightObjects()->isInflight(fence))
    {
        m_fences.insert(fence);
    }
    else
    {
        m_fences.erase(fence);
        destroy(fence);
    }
}

void VulkanDeleter::destroy(VkBuffer buffer, VulkanMemory memory)
{
    m_device->getResourceAllocator().destroyBufferResource({ buffer, memory });
}

void VulkanDeleter::destroy(VkImage image, VulkanMemory memory)
{
    m_device->getResourceAllocator().destroyTextureResource({ image, memory });
}

void VulkanDeleter::destroy(VkCommandBuffer commandBuffer)
{
    m_device->getCommandPool()->release(commandBuffer);
}

void VulkanDeleter::destroy(VkImageView imageView)
{
    m_device->vkAPI.DestroyImageView(m_device->getVkDevice(), imageView, nullptr);
}
void VulkanDeleter::destroy(VkSemaphore semaphore)
{
    m_device->getSemaphorePool()->release(semaphore);
}

void VulkanDeleter::destroy(VkSampler sampler)
{
    m_device->vkAPI.DestroySampler(m_device->getVkDevice(), sampler, nullptr);
}
void VulkanDeleter::destroy(VkPipeline pipeline)
{
    m_device->vkAPI.DestroyPipeline(m_device->getVkDevice(), pipeline, nullptr);
}

void VulkanDeleter::destroy(VkPipelineLayout pipelineLayout)
{
    m_device->vkAPI.DestroyPipelineLayout(m_device->getVkDevice(), pipelineLayout, nullptr);
}

void VulkanDeleter::destroy(VkDescriptorSet descriptorSet)
{
    m_device->vkAPI.FreeDescriptorSets(m_device->getVkDevice(), m_device->getVkDescriptorPool(), 1, &descriptorSet);
}

void VulkanDeleter::destroy(VkDescriptorSetLayout descriptorSetLayout)
{
    m_device->vkAPI.DestroyDescriptorSetLayout(m_device->getVkDevice(), descriptorSetLayout, nullptr);
}

void VulkanDeleter::destroy(VkFramebuffer framebuffer)
{
    m_device->vkAPI.DestroyFramebuffer(m_device->getVkDevice(), framebuffer, nullptr);
}

void VulkanDeleter::destroy(VkRenderPass renderPass)
{
    m_device->vkAPI.DestroyRenderPass(m_device->getVkDevice(), renderPass, nullptr);
}

void VulkanDeleter::destroy(VkFence fence)
{
    m_device->getFencePool()->release(fence);
}

} // namespace jipu