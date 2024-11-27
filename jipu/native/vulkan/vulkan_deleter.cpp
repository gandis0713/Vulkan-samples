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
    m_device->getInflightObjects()->subscribe(this, [this](VkFence fence, VulkanInflightObject object) {
        for (auto commandBuffer : object.commandBuffers)
        {
            if (contains(commandBuffer))
            {
                safeDestroy(commandBuffer);
            }
        }

        for (auto [buffer, memory] : object.buffers)
        {
            if (contains(buffer))
            {
                safeDestroy(buffer, memory);
            }
        }

        for (auto [image, memory] : object.images)
        {
            if (contains(image))
            {
                safeDestroy(image, memory);
            }
        }

        for (auto imageView : object.imageViews)
        {
            if (contains(imageView))
            {
                safeDestroy(imageView);
            }
        }

        for (auto semaphore : object.semaphores)
        {
            if (contains(semaphore))
            {
                safeDestroy(semaphore);
            }
        }

        for (auto sampler : object.samplers)
        {
            if (contains(sampler))
            {
                safeDestroy(sampler);
            }
        }

        for (auto pipeline : object.pipelines)
        {
            if (contains(pipeline))
            {
                safeDestroy(pipeline);
            }
        }

        for (auto pipelineLayout : object.pipelineLayouts)
        {
            if (contains(pipelineLayout))
            {
                safeDestroy(pipelineLayout);
            }
        }

        for (auto shaderModule : object.shaderModules)
        {
            if (contains(shaderModule))
            {
                safeDestroy(shaderModule);
            }
        }

        for (auto descriptorSet : object.descriptorSet)
        {
            if (contains(descriptorSet))
            {
                safeDestroy(descriptorSet);
            }
        }

        for (auto descriptorSetLayout : object.descriptorSetLayouts)
        {
            if (contains(descriptorSetLayout))
            {
                safeDestroy(descriptorSetLayout);
            }
        }

        for (auto framebuffer : object.framebuffers)
        {
            if (contains(framebuffer))
            {
                safeDestroy(framebuffer);
            }
        }

        for (auto renderPass : object.renderPasses)
        {
            if (contains(renderPass))
            {
                safeDestroy(renderPass);
            }
        }

        if (contains(fence))
        {
            safeDestroy(fence);
        }
    });
}

VulkanDeleter::~VulkanDeleter()
{
    m_device->getInflightObjects()->unsubscribe(this);

    std::unordered_map<VkBuffer, VulkanMemory> buffers{};
    std::unordered_map<VkImage, VulkanMemory> images{};
    std::unordered_set<VkCommandBuffer> commandBuffers{};
    std::unordered_set<VkImageView> imageViews{};
    std::unordered_set<VkSemaphore> semaphores{};
    std::unordered_set<VkSampler> samplers{};
    std::unordered_set<VkPipeline> pipelines{};
    std::unordered_set<VkPipelineLayout> pipelineLayouts{};
    std::unordered_set<VkShaderModule> shaderModules{};
    std::unordered_set<VkDescriptorSet> descriptorSets{};
    std::unordered_set<VkDescriptorSetLayout> descriptorSetLayouts{};
    std::unordered_set<VkFramebuffer> framebuffers{};
    std::unordered_set<VkRenderPass> renderPasses{};
    std::unordered_set<VkFence> fences{};

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        buffers = std::move(m_buffers);
        images = std::move(m_images);
        commandBuffers = std::move(m_commandBuffers);
        imageViews = std::move(m_imageViews);
        semaphores = std::move(m_semaphores);
        samplers = std::move(m_samplers);
        pipelines = std::move(m_pipelines);
        pipelineLayouts = std::move(m_pipelineLayouts);
        shaderModules = std::move(m_shaderModules);
        descriptorSets = std::move(m_descriptorSets);
        descriptorSetLayouts = std::move(m_descriptorSetLayouts);
        framebuffers = std::move(m_framebuffers);
        renderPasses = std::move(m_renderPasses);
        fences = std::move(m_fences);
    }

    for (auto [buffer, memory] : buffers)
    {
        destroy(buffer, memory);
    }

    for (auto [image, memory] : images)
    {
        destroy(image, memory);
    }

    for (auto commandBuffer : commandBuffers)
    {
        destroy(commandBuffer);
    }

    for (auto imageView : imageViews)
    {
        destroy(imageView);
    }

    for (auto semaphore : semaphores)
    {
        destroy(semaphore);
    }

    for (auto sampler : samplers)
    {
        destroy(sampler);
    }

    for (auto pipeline : pipelines)
    {
        destroy(pipeline);
    }

    for (auto pipelineLayout : pipelineLayouts)
    {
        destroy(pipelineLayout);
    }

    for (auto shaderModule : shaderModules)
    {
        destroy(shaderModule);
    }

    for (auto descriptorSet : descriptorSets)
    {
        destroy(descriptorSet);
    }

    for (auto descriptorSetLayout : descriptorSetLayouts)
    {
        destroy(descriptorSetLayout);
    }

    for (auto framebuffer : framebuffers)
    {
        destroy(framebuffer);
    }

    for (auto renderPass : renderPasses)
    {
        destroy(renderPass);
    }

    for (auto fence : fences)
    {
        destroy(fence);
    }
}

void VulkanDeleter::safeDestroy(VkBuffer buffer, VulkanMemory memory)
{
    if (m_device->getInflightObjects()->isInflight(buffer))
    {
        insert(buffer, memory);
    }
    else
    {
        erase(buffer);
        destroy(buffer, memory);
    }
}

void VulkanDeleter::safeDestroy(VkImage image, VulkanMemory memory)
{
    if (m_device->getInflightObjects()->isInflight(image))
    {
        insert(image, memory);
    }
    else
    {
        erase(image);
        destroy(image, memory);
    }
}

void VulkanDeleter::safeDestroy(VkCommandBuffer commandBuffer)
{
    if (m_device->getInflightObjects()->isInflight(commandBuffer))
    {
        insert(commandBuffer);
    }
    else
    {
        erase(commandBuffer);
        destroy(commandBuffer);
    }
}

void VulkanDeleter::safeDestroy(VkImageView imageView)
{
    if (m_device->getInflightObjects()->isInflight(imageView))
    {
        insert(imageView);
    }
    else
    {
        erase(imageView);
        destroy(imageView);
    }

    // invalidate framebuffer cache
    {
        auto framebufferCache = m_device->getFramebufferCache();
        framebufferCache->invalidate(imageView);
    }
}
void VulkanDeleter::safeDestroy(VkSemaphore semaphore)
{
    if (m_device->getInflightObjects()->isInflight(semaphore))
    {
        insert(semaphore);
    }
    else
    {
        erase(semaphore);
        destroy(semaphore);
    }
}

void VulkanDeleter::safeDestroy(VkSampler sampler)
{
    if (m_device->getInflightObjects()->isInflight(sampler))
    {
        insert(sampler);
    }
    else
    {
        erase(sampler);
        destroy(sampler);
    }
}
void VulkanDeleter::safeDestroy(VkPipeline pipeline)
{
    if (m_device->getInflightObjects()->isInflight(pipeline))
    {
        insert(pipeline);
    }
    else
    {
        erase(pipeline);
        destroy(pipeline);
    }
}

void VulkanDeleter::safeDestroy(VkPipelineLayout pipelineLayout)
{
    if (m_device->getInflightObjects()->isInflight(pipelineLayout))
    {
        insert(pipelineLayout);
    }
    else
    {
        erase(pipelineLayout);
        destroy(pipelineLayout);
    }
}

void VulkanDeleter::safeDestroy(VkShaderModule shaderModule)
{
    if (m_device->getInflightObjects()->isInflight(shaderModule))
    {
        insert(shaderModule);
    }
    else
    {
        erase(shaderModule);
        destroy(shaderModule);
    }
}

void VulkanDeleter::safeDestroy(VkDescriptorSet descriptorSet)
{
    if (m_device->getInflightObjects()->isInflight(descriptorSet))
    {
        insert(descriptorSet);
    }
    else
    {
        erase(descriptorSet);
        destroy(descriptorSet);
    }
}

void VulkanDeleter::safeDestroy(VkDescriptorSetLayout descriptorSetLayout)
{
    if (m_device->getInflightObjects()->isInflight(descriptorSetLayout))
    {
        insert(descriptorSetLayout);
    }
    else
    {
        erase(descriptorSetLayout);
        destroy(descriptorSetLayout);
    }
}

void VulkanDeleter::safeDestroy(VkFramebuffer framebuffer)
{
    if (m_device->getInflightObjects()->isInflight(framebuffer))
    {
        insert(framebuffer);
    }
    else
    {
        erase(framebuffer);
        destroy(framebuffer);
    }
}

void VulkanDeleter::safeDestroy(VkRenderPass renderPass)
{
    if (m_device->getInflightObjects()->isInflight(renderPass))
    {
        insert(renderPass);
    }
    else
    {
        erase(renderPass);
        destroy(renderPass);
    }

    // invalidate framebuffer cache
    {
        auto framebufferCache = m_device->getFramebufferCache();
        framebufferCache->invalidate(renderPass);
    }
}

void VulkanDeleter::safeDestroy(VkFence fence)
{
    if (m_device->getInflightObjects()->isInflight(fence))
    {
        insert(fence);
    }
    else
    {
        erase(fence);
        destroy(fence);
    }
}

void VulkanDeleter::destroy(VkBuffer buffer, VulkanMemory memory)
{
    m_device->getResourceAllocator()->destroyBufferResource({ buffer, memory });
}

void VulkanDeleter::destroy(VkImage image, VulkanMemory memory)
{
    m_device->getResourceAllocator()->destroyTextureResource({ image, memory });
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

void VulkanDeleter::destroy(VkShaderModule shaderModule)
{
    m_device->vkAPI.DestroyShaderModule(m_device->getVkDevice(), shaderModule, nullptr);
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

void VulkanDeleter::insert(VkBuffer buffer, VulkanMemory memory)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_buffers.insert({ buffer, memory });
}

void VulkanDeleter::insert(VkImage image, VulkanMemory memory)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_images.insert({ image, memory });
}

void VulkanDeleter::insert(VkCommandBuffer commandBuffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_commandBuffers.insert(commandBuffer);
}

void VulkanDeleter::insert(VkImageView imageView)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_imageViews.insert(imageView);
}

void VulkanDeleter::insert(VkSemaphore semaphore)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_semaphores.insert(semaphore);
}

void VulkanDeleter::insert(VkSampler sampler)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_samplers.insert(sampler);
}

void VulkanDeleter::insert(VkPipeline pipeline)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_pipelines.insert(pipeline);
}

void VulkanDeleter::insert(VkPipelineLayout pipelineLayout)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_pipelineLayouts.insert(pipelineLayout);
}

void VulkanDeleter::insert(VkShaderModule shaderModule)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_shaderModules.insert(shaderModule);
}

void VulkanDeleter::insert(VkDescriptorSet descriptorSet)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_descriptorSets.insert(descriptorSet);
}

void VulkanDeleter::insert(VkDescriptorSetLayout descriptorSetLayout)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_descriptorSetLayouts.insert(descriptorSetLayout);
}

void VulkanDeleter::insert(VkFramebuffer framebuffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_framebuffers.insert(framebuffer);
}

void VulkanDeleter::insert(VkRenderPass renderPass)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_renderPasses.insert(renderPass);
}

void VulkanDeleter::insert(VkFence fence)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_fences.insert(fence);
}

void VulkanDeleter::erase(VkBuffer buffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_buffers.erase(buffer);
}

void VulkanDeleter::erase(VkImage image)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_images.erase(image);
}

void VulkanDeleter::erase(VkCommandBuffer commandBuffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_commandBuffers.erase(commandBuffer);
}

void VulkanDeleter::erase(VkImageView imageView)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_imageViews.erase(imageView);
}

void VulkanDeleter::erase(VkSemaphore semaphore)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_semaphores.erase(semaphore);
}

void VulkanDeleter::erase(VkSampler sampler)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_samplers.erase(sampler);
}

void VulkanDeleter::erase(VkPipeline pipeline)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_pipelines.erase(pipeline);
}

void VulkanDeleter::erase(VkPipelineLayout pipelineLayout)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_pipelineLayouts.erase(pipelineLayout);
}

void VulkanDeleter::erase(VkShaderModule shaderModule)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_shaderModules.erase(shaderModule);
}

void VulkanDeleter::erase(VkDescriptorSet descriptorSet)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_descriptorSets.erase(descriptorSet);
}

void VulkanDeleter::erase(VkDescriptorSetLayout descriptorSetLayout)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_descriptorSetLayouts.erase(descriptorSetLayout);
}

void VulkanDeleter::erase(VkFramebuffer framebuffer)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_framebuffers.erase(framebuffer);
}

void VulkanDeleter::erase(VkRenderPass renderPass)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_renderPasses.erase(renderPass);
}

void VulkanDeleter::erase(VkFence fence)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_fences.erase(fence);
}

bool VulkanDeleter::contains(VkBuffer buffer) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_buffers.contains(buffer);
}

bool VulkanDeleter::contains(VkImage image) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_images.contains(image);
}

bool VulkanDeleter::contains(VkCommandBuffer commandBuffer) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_commandBuffers.contains(commandBuffer);
}

bool VulkanDeleter::contains(VkImageView imageView) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_imageViews.contains(imageView);
}

bool VulkanDeleter::contains(VkSemaphore semaphore) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_semaphores.contains(semaphore);
}

bool VulkanDeleter::contains(VkSampler sampler) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_samplers.contains(sampler);
}

bool VulkanDeleter::contains(VkPipeline pipeline) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_pipelines.contains(pipeline);
}

bool VulkanDeleter::contains(VkPipelineLayout pipelineLayout) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_pipelineLayouts.contains(pipelineLayout);
}

bool VulkanDeleter::contains(VkShaderModule shaderModule) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_shaderModules.contains(shaderModule);
}

bool VulkanDeleter::contains(VkDescriptorSet descriptorSet) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_descriptorSets.contains(descriptorSet);
}

bool VulkanDeleter::contains(VkDescriptorSetLayout descriptorSetLayout) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_descriptorSetLayouts.contains(descriptorSetLayout);
}

bool VulkanDeleter::contains(VkFramebuffer framebuffer) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_framebuffers.contains(framebuffer);
}

bool VulkanDeleter::contains(VkRenderPass renderPass) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_renderPasses.contains(renderPass);
}

bool VulkanDeleter::contains(VkFence fence) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_fences.contains(fence);
}

} // namespace jipu