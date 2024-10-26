#include "vulkan_binding_group.h"
#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_sampler.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

struct VulkanBindingGroupDescriptor
{
    VulkanBindingGroupLayout* layout = nullptr;
    std::vector<VkDescriptorBufferInfo> buffers{};
    std::vector<VkDescriptorImageInfo> samplers{};
    std::vector<VkDescriptorImageInfo> textures{};
};

VulkanBindingGroupDescriptor generateVulkanBindingGroupDescriptor(const BindingGroupDescriptor& descriptor)
{
    VulkanBindingGroupDescriptor vkdescriptor{
        .layout = downcast(descriptor.layout)
    };

    const uint64_t bufferSize = descriptor.buffers.size();
    const uint64_t samplerSize = descriptor.samplers.size();
    const uint64_t textureSize = descriptor.textures.size();

    vkdescriptor.buffers.resize(bufferSize);
    for (auto i = 0; i < bufferSize; ++i)
    {
        const BufferBinding& buffer = descriptor.buffers[i];

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = downcast(buffer.buffer)->getVkBuffer();
        bufferInfo.offset = buffer.offset;
        bufferInfo.range = buffer.size;

        vkdescriptor.buffers[i] = bufferInfo;
    }

    vkdescriptor.samplers.resize(samplerSize);
    for (auto i = 0; i < samplerSize; ++i)
    {
        const SamplerBinding& sampler = descriptor.samplers[i];

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = downcast(sampler.sampler)->getVkSampler();

        vkdescriptor.samplers[i] = imageInfo;
    }

    vkdescriptor.textures.resize(textureSize);
    // update texture
    for (auto i = 0; i < textureSize; ++i)
    {
        const TextureBinding& texture = descriptor.textures[i];

        auto vulkanTextureView = downcast(texture.textureView);
        auto vulkanTexture = downcast(vulkanTextureView->getTexture());

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageView = vulkanTextureView->getVkImageView();
        imageInfo.imageLayout = vulkanTexture->getFinalLayout();

        vkdescriptor.textures[i] = imageInfo;
    }

    return vkdescriptor;
}

VulkanBindingGroup::VulkanBindingGroup(VulkanDevice& device, const BindingGroupDescriptor& descriptor)
    : BindingGroup()
    , m_device(device)
    , m_descriptor(descriptor)
{
    auto vkdescriptor = generateVulkanBindingGroupDescriptor(descriptor);

    auto& vulkanDevice = downcast(device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;
    auto vulkanBindingGroupLayout = downcast(m_descriptor.layout);
    auto descriptorSetLayout = vulkanBindingGroupLayout->getVkDescriptorSetLayout();

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = downcast(device).getVkDescriptorPool();
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

    VkResult result = vkAPI.AllocateDescriptorSets(vulkanDevice.getVkDevice(), &descriptorSetAllocateInfo, &m_descriptorSet);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets.");
    }

    const uint64_t bufferSize = descriptor.buffers.size();
    const uint64_t samplerSize = descriptor.samplers.size();
    const uint64_t textureSize = descriptor.textures.size();

    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.resize(bufferSize + samplerSize + textureSize);

    for (auto i = 0; i < bufferSize; ++i)
    {
        const VkDescriptorBufferInfo& buffer = vkdescriptor.buffers[i];
        auto bufferLayout = vulkanBindingGroupLayout->getBufferDescriptorSetLayout(i);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSet;
        descriptorWrite.dstBinding = bufferLayout.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = bufferLayout.descriptorType;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = &buffer;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        descriptorWrites[i] = descriptorWrite;
    }

    for (auto i = 0; i < samplerSize; ++i)
    {
        const VkDescriptorImageInfo& sampler = vkdescriptor.samplers[i];
        auto samplerLayout = vulkanBindingGroupLayout->getSamplerDescriptorSetLayout(i);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSet;
        descriptorWrite.dstBinding = samplerLayout.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = samplerLayout.descriptorType;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = &sampler;
        descriptorWrite.pTexelBufferView = nullptr;

        descriptorWrites[bufferSize + i] = descriptorWrite;
    }

    for (auto i = 0; i < textureSize; ++i)
    {
        const VkDescriptorImageInfo& texture = vkdescriptor.textures[i];
        auto textureLayout = vulkanBindingGroupLayout->getTextureDescriptorSetLayout(i);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSet;
        descriptorWrite.dstBinding = textureLayout.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = textureLayout.descriptorType;
        descriptorWrite.descriptorCount = 1;

        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = &texture;
        descriptorWrite.pTexelBufferView = nullptr;

        descriptorWrites[bufferSize + samplerSize + i] = descriptorWrite;
    }

    vkAPI.UpdateDescriptorSets(vulkanDevice.getVkDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

VulkanBindingGroup::~VulkanBindingGroup()
{
    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    vkAPI.FreeDescriptorSets(vulkanDevice.getVkDevice(), vulkanDevice.getVkDescriptorPool(), 1, &m_descriptorSet);
}

BindingGroupLayout* VulkanBindingGroup::getLayout() const
{
    return downcast(m_descriptor.layout);
}

const std::vector<BufferBinding>& VulkanBindingGroup::getBufferBindings() const
{
    return m_descriptor.buffers;
}

const std::vector<SamplerBinding>& VulkanBindingGroup::getSmaplerBindings() const
{
    return m_descriptor.samplers;
}

const std::vector<TextureBinding>& VulkanBindingGroup::getTextureBindings() const
{
    return m_descriptor.textures;
}

VkDescriptorSet VulkanBindingGroup::getVkDescriptorSet() const
{
    return m_descriptorSet;
}

} // namespace jipu