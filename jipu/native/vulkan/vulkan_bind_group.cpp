#include "vulkan_bind_group.h"
#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_sampler.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

struct VulkanBindGroupDescriptor
{
    VulkanBindGroupLayout* layout = nullptr;
    std::vector<VkDescriptorBufferInfo> buffers{};
    std::vector<VkDescriptorImageInfo> samplers{};
    std::vector<VkDescriptorImageInfo> textures{};
};

VulkanBindGroupDescriptor generateVulkanBindGroupDescriptor(const BindGroupDescriptor& descriptor)
{
    VulkanBindGroupDescriptor vkdescriptor{
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

    auto textureDescriptorSetLayouts = downcast(descriptor.layout)->getTextureDescriptorSetLayouts();
    // update texture

    for (auto i = 0; i < textureSize; ++i)
    {
        const TextureBinding& texture = descriptor.textures[i];

        auto vulkanTextureView = downcast(texture.textureView);
        auto vulkanTexture = downcast(vulkanTextureView->getTexture());

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageView = vulkanTextureView->getVkImageView();

        // set image layout by shader stage
        // TODO: image layout
        {
            auto stageFlags = textureDescriptorSetLayouts[i].stageFlags;
            if (stageFlags & VK_SHADER_STAGE_COMPUTE_BIT)
            {
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
            else if (stageFlags & VK_SHADER_STAGE_VERTEX_BIT)
            {
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            else if (stageFlags & VK_SHADER_STAGE_FRAGMENT_BIT)
            {
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                if (vulkanTexture->getFormat() == TextureFormat::kDepth32Float)
                {
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
            }
        }

        vkdescriptor.textures[i] = imageInfo;
    }

    return vkdescriptor;
}

VulkanBindGroup::VulkanBindGroup(VulkanDevice* device, const BindGroupDescriptor& descriptor)
    : BindGroup()
    , m_device(device)
    , m_descriptor(descriptor)
    , m_layoutInfo(downcast(m_descriptor.layout)->getInfo())
{
    auto vkdescriptor = generateVulkanBindGroupDescriptor(descriptor);

    auto vulkanDevice = downcast(device);
    const VulkanAPI& vkAPI = vulkanDevice->vkAPI;
    auto vulkanBindGroupLayout = downcast(m_descriptor.layout);
    m_descriptorSet = m_device->getDescriptorPool()->allocate(vulkanBindGroupLayout);

    const uint64_t bufferSize = descriptor.buffers.size();
    const uint64_t samplerSize = descriptor.samplers.size();
    const uint64_t textureSize = descriptor.textures.size();

    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.resize(bufferSize + samplerSize + textureSize);

    for (auto i = 0; i < bufferSize; ++i)
    {
        const VkDescriptorBufferInfo& buffer = vkdescriptor.buffers[i];
        auto bufferLayout = vulkanBindGroupLayout->getBufferDescriptorSetLayout(i);

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
        auto samplerLayout = vulkanBindGroupLayout->getSamplerDescriptorSetLayout(i);

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
        auto textureLayout = vulkanBindGroupLayout->getTextureDescriptorSetLayout(i);

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

    vkAPI.UpdateDescriptorSets(vulkanDevice->getVkDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

VulkanBindGroup::~VulkanBindGroup()
{
    m_device->getDeleter()->safeDestroy(m_descriptorSet);
}

VulkanDevice* VulkanBindGroup::getDevice() const
{
    return m_device;
}

const std::vector<BufferBinding>& VulkanBindGroup::getBufferBindings() const
{
    return m_descriptor.buffers;
}

const std::vector<SamplerBinding>& VulkanBindGroup::getSmaplerBindings() const
{
    return m_descriptor.samplers;
}

const std::vector<TextureBinding>& VulkanBindGroup::getTextureBindings() const
{
    return m_descriptor.textures;
}

const VulkanBindGroupLayoutInfo& VulkanBindGroup::getLayoutInfo() const
{
    return m_layoutInfo;
}

const std::vector<BufferBindingLayout>& VulkanBindGroup::getBufferLayouts() const
{
    return m_layoutInfo.buffers;
}

const std::vector<SamplerBindingLayout>& VulkanBindGroup::getSamplerLayouts() const
{
    return m_layoutInfo.samplers;
}

const std::vector<TextureBindingLayout>& VulkanBindGroup::getTextureLayouts() const
{
    return m_layoutInfo.textures;
}

VkDescriptorSet VulkanBindGroup::getVkDescriptorSet() const
{
    return m_descriptorSet;
}

} // namespace jipu