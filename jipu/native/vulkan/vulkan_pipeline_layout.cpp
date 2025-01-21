#include "vulkan_pipeline_layout.h"

#include "vulkan_bind_group_layout.h"
#include "vulkan_device.h"

#include "jipu/common/hash.h"
#include <stdexcept>

namespace jipu
{

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device, const PipelineLayoutDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    m_info.bindGroupLayoutInfos.resize(m_descriptor.layouts.size());

    for (uint32_t i = 0; i < m_info.bindGroupLayoutInfos.size(); ++i)
    {
        auto bindGroupLayout = downcast(m_descriptor.layouts[i]);
        m_info.bindGroupLayoutInfos[i] = bindGroupLayout->getInfo();
    }
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    // do not destroy VkPipelineLayout here. because it is managed by VulkanPipelineLayoutCache.
}

VkPipelineLayout VulkanPipelineLayout::getVkPipelineLayout() const
{
    VulkanPipelineLayoutMetaData metaData{
        .info = m_info,
    };
    return m_device->getPipelineLayoutCache()->getVkPipelineLayout(metaData);
}

const VulkanPipelineLayoutInfo& VulkanPipelineLayout::getInfo() const
{
    return m_info;
}

// VulkanPipelineLayoutCache

size_t VulkanPipelineLayoutCache::Functor::operator()(const VulkanPipelineLayoutMetaData& metaData) const
{
    size_t hash = 0;

    for (const auto& bindGroupLayoutInfos : metaData.info.bindGroupLayoutInfos)
    {
        for (const auto& buffer : bindGroupLayoutInfos.buffers)
        {
            combineHash(hash, buffer.dynamicOffset);
            combineHash(hash, buffer.index);
            combineHash(hash, buffer.stages);
            combineHash(hash, buffer.type);
        }

        for (const auto& sampler : bindGroupLayoutInfos.samplers)
        {
            combineHash(hash, sampler.index);
            combineHash(hash, sampler.stages);
        }

        for (const auto& texture : bindGroupLayoutInfos.textures)
        {
            combineHash(hash, texture.index);
            combineHash(hash, texture.stages);
        }

        for (const auto& storageTexture : bindGroupLayoutInfos.storageTextures)
        {
            combineHash(hash, storageTexture.index);
            combineHash(hash, storageTexture.stages);
            combineHash(hash, storageTexture.type);
        }
    }

    return hash;
}

bool VulkanPipelineLayoutCache::Functor::operator()(const VulkanPipelineLayoutMetaData& lhs,
                                                    const VulkanPipelineLayoutMetaData& rhs) const
{
    if (lhs.info.bindGroupLayoutInfos.size() != rhs.info.bindGroupLayoutInfos.size())
    {
        return false;
    }

    for (auto i = 0; i < lhs.info.bindGroupLayoutInfos.size(); ++i)
    {
        if (lhs.info.bindGroupLayoutInfos[i].buffers.size() != rhs.info.bindGroupLayoutInfos[i].buffers.size() ||
            lhs.info.bindGroupLayoutInfos[i].samplers.size() != rhs.info.bindGroupLayoutInfos[i].samplers.size() ||
            lhs.info.bindGroupLayoutInfos[i].textures.size() != rhs.info.bindGroupLayoutInfos[i].textures.size() ||
            lhs.info.bindGroupLayoutInfos[i].storageTextures.size() != rhs.info.bindGroupLayoutInfos[i].storageTextures.size())
        {
            return false;
        }

        for (auto j = 0; j < lhs.info.bindGroupLayoutInfos[i].buffers.size(); ++j)
        {
            if (lhs.info.bindGroupLayoutInfos[i].buffers[j].dynamicOffset != rhs.info.bindGroupLayoutInfos[i].buffers[j].dynamicOffset ||
                lhs.info.bindGroupLayoutInfos[i].buffers[j].index != rhs.info.bindGroupLayoutInfos[i].buffers[j].index ||
                lhs.info.bindGroupLayoutInfos[i].buffers[j].stages != rhs.info.bindGroupLayoutInfos[i].buffers[j].stages ||
                lhs.info.bindGroupLayoutInfos[i].buffers[j].type != rhs.info.bindGroupLayoutInfos[i].buffers[j].type)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.info.bindGroupLayoutInfos[i].samplers.size(); ++j)
        {
            if (lhs.info.bindGroupLayoutInfos[i].samplers[j].index != rhs.info.bindGroupLayoutInfos[i].samplers[j].index ||
                lhs.info.bindGroupLayoutInfos[i].samplers[j].stages != rhs.info.bindGroupLayoutInfos[i].samplers[j].stages)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.info.bindGroupLayoutInfos[i].textures.size(); ++j)
        {
            if (lhs.info.bindGroupLayoutInfos[i].textures[j].index != rhs.info.bindGroupLayoutInfos[i].textures[j].index ||
                lhs.info.bindGroupLayoutInfos[i].textures[j].stages != rhs.info.bindGroupLayoutInfos[i].textures[j].stages)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.info.bindGroupLayoutInfos[i].storageTextures.size(); ++j)
        {
            if (lhs.info.bindGroupLayoutInfos[i].storageTextures[j].index != rhs.info.bindGroupLayoutInfos[i].storageTextures[j].index ||
                lhs.info.bindGroupLayoutInfos[i].storageTextures[j].stages != rhs.info.bindGroupLayoutInfos[i].storageTextures[j].stages ||
                lhs.info.bindGroupLayoutInfos[i].storageTextures[j].type != rhs.info.bindGroupLayoutInfos[i].storageTextures[j].type)
            {
                return false;
            }
        }
    }

    return true;
}

VulkanPipelineLayoutCache::VulkanPipelineLayoutCache(VulkanDevice* device)
    : m_device(device)
{
}

VulkanPipelineLayoutCache::~VulkanPipelineLayoutCache()
{
    clear();
}

VkPipelineLayout VulkanPipelineLayoutCache::getVkPipelineLayout(const VulkanPipelineLayoutMetaData& metaData)
{
    auto it = m_pipelineLayouts.find(metaData);
    if (it != m_pipelineLayouts.end())
    {
        return it->second;
    }

    std::vector<VkDescriptorSetLayout> layouts{};
    layouts.resize(metaData.info.bindGroupLayoutInfos.size());
    for (uint32_t i = 0; i < layouts.size(); ++i)
    {
        VulkanBindGroupLayoutMetaData bindGroupLayoutMetaData{
            .info = metaData.info.bindGroupLayoutInfos[i],
        };
        layouts[i] = m_device->getBindGroupLayoutCache()->getVkDescriptorSetLayout(bindGroupLayoutMetaData);
    }

    VkPipelineLayoutCreateInfo createInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                           .setLayoutCount = static_cast<uint32_t>(layouts.size()),
                                           .pSetLayouts = layouts.data() };

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkResult result = m_device->vkAPI.CreatePipelineLayout(m_device->getVkDevice(), &createInfo, nullptr, &pipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkPipelineLayout");
    }

    m_pipelineLayouts.insert({ metaData, pipelineLayout });

    return pipelineLayout;
}

void VulkanPipelineLayoutCache::clear()
{
    for (auto& [descriptor, pipelineLayout] : m_pipelineLayouts)
    {
        m_device->getDeleter()->safeDestroy(pipelineLayout);
    }

    m_pipelineLayouts.clear();
}

} // namespace jipu