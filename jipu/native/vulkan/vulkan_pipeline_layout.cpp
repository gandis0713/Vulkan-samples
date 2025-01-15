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
    m_pipelineLayout = m_device->getPipelineLayoutCache()->getVkPipelineLayout(descriptor);
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    // do not destroy VkPipelineLayout here. because it is managed by VulkanPipelineLayoutCache.
}

VkPipelineLayout VulkanPipelineLayout::getVkPipelineLayout() const
{
    return m_pipelineLayout;
}

PipelineLayoutMetaInfo VulkanPipelineLayout::getLayoutInfo() const
{
    PipelineLayoutMetaInfo layoutInfo{};
    layoutInfo.bindGroupLayoutMetaInfos.resize(m_descriptor.layouts.size());

    for (uint32_t i = 0; i < layoutInfo.bindGroupLayoutMetaInfos.size(); ++i)
    {
        layoutInfo.bindGroupLayoutMetaInfos[i] = BindGroupLayoutMetaInfo{
            .buffers = downcast(m_descriptor.layouts[i])->getBufferBindingLayouts(),
            .samplers = downcast(m_descriptor.layouts[i])->getSamplerBindingLayouts(),
            .textures = downcast(m_descriptor.layouts[i])->getTextureBindingLayouts(),
        };
    }

    return layoutInfo;
}

// VulkanPipelineLayoutCache

VulkanPipelineLayoutCache::VulkanPipelineLayoutCache(VulkanDevice* device)
    : m_device(device)
{
}

VulkanPipelineLayoutCache::~VulkanPipelineLayoutCache()
{
    clear();
}

VkPipelineLayout VulkanPipelineLayoutCache::getVkPipelineLayout(const PipelineLayoutDescriptor& descriptor)
{
    PipelineLayoutMetaInfo layoutInfo{};
    layoutInfo.bindGroupLayoutMetaInfos.resize(descriptor.layouts.size());

    for (uint32_t i = 0; i < layoutInfo.bindGroupLayoutMetaInfos.size(); ++i)
    {
        layoutInfo.bindGroupLayoutMetaInfos[i] = BindGroupLayoutMetaInfo{
            .buffers = downcast(descriptor.layouts[i])->getBufferBindingLayouts(),
            .samplers = downcast(descriptor.layouts[i])->getSamplerBindingLayouts(),
            .textures = downcast(descriptor.layouts[i])->getTextureBindingLayouts(),
        };
    }

    return getVkPipelineLayout(layoutInfo);
}

VkPipelineLayout VulkanPipelineLayoutCache::getVkPipelineLayout(const PipelineLayoutMetaInfo& layoutInfo)
{
    auto it = m_pipelineLayouts.find(layoutInfo);
    if (it != m_pipelineLayouts.end())
    {
        return it->second;
    }

    std::vector<VkDescriptorSetLayout> layouts{};
    layouts.resize(layoutInfo.bindGroupLayoutMetaInfos.size());
    for (uint32_t i = 0; i < layouts.size(); ++i)
    {
        layouts[i] = m_device->getBindGroupLayoutCache()->getVkDescriptorSetLayout(layoutInfo.bindGroupLayoutMetaInfos[i]);
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

    m_pipelineLayouts.insert({ layoutInfo, pipelineLayout });

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

size_t VulkanPipelineLayoutCache::Functor::operator()(const PipelineLayoutMetaInfo& layoutInfo) const
{
    size_t hash = 0;

    for (const auto& bindGroupLayoutMetaInfo : layoutInfo.bindGroupLayoutMetaInfos)
    {
        for (const auto& buffer : bindGroupLayoutMetaInfo.buffers)
        {
            combineHash(hash, buffer.dynamicOffset);
            combineHash(hash, buffer.index);
            combineHash(hash, buffer.stages);
            combineHash(hash, buffer.type);
        }

        for (const auto& sampler : bindGroupLayoutMetaInfo.samplers)
        {
            combineHash(hash, sampler.index);
            combineHash(hash, sampler.stages);
        }

        for (const auto& texture : bindGroupLayoutMetaInfo.textures)
        {
            combineHash(hash, texture.index);
            combineHash(hash, texture.stages);
        }
    }

    return hash;
}

bool VulkanPipelineLayoutCache::Functor::operator()(const PipelineLayoutMetaInfo& lhs,
                                                    const PipelineLayoutMetaInfo& rhs) const
{
    if (lhs.bindGroupLayoutMetaInfos.size() != rhs.bindGroupLayoutMetaInfos.size())
    {
        return false;
    }

    for (auto i = 0; i < lhs.bindGroupLayoutMetaInfos.size(); ++i)
    {
        if (lhs.bindGroupLayoutMetaInfos[i].buffers.size() != rhs.bindGroupLayoutMetaInfos[i].buffers.size() ||
            lhs.bindGroupLayoutMetaInfos[i].samplers.size() != rhs.bindGroupLayoutMetaInfos[i].samplers.size() ||
            lhs.bindGroupLayoutMetaInfos[i].textures.size() != rhs.bindGroupLayoutMetaInfos[i].textures.size())
        {
            return false;
        }

        for (auto j = 0; j < lhs.bindGroupLayoutMetaInfos[i].buffers.size(); ++j)
        {
            if (lhs.bindGroupLayoutMetaInfos[i].buffers[j].dynamicOffset != rhs.bindGroupLayoutMetaInfos[i].buffers[j].dynamicOffset ||
                lhs.bindGroupLayoutMetaInfos[i].buffers[j].index != rhs.bindGroupLayoutMetaInfos[i].buffers[j].index ||
                lhs.bindGroupLayoutMetaInfos[i].buffers[j].stages != rhs.bindGroupLayoutMetaInfos[i].buffers[j].stages ||
                lhs.bindGroupLayoutMetaInfos[i].buffers[j].type != rhs.bindGroupLayoutMetaInfos[i].buffers[j].type)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.bindGroupLayoutMetaInfos[i].samplers.size(); ++j)
        {
            if (lhs.bindGroupLayoutMetaInfos[i].samplers[j].index != rhs.bindGroupLayoutMetaInfos[i].samplers[j].index ||
                lhs.bindGroupLayoutMetaInfos[i].samplers[j].stages != rhs.bindGroupLayoutMetaInfos[i].samplers[j].stages)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.bindGroupLayoutMetaInfos[i].textures.size(); ++j)
        {
            if (lhs.bindGroupLayoutMetaInfos[i].textures[j].index != rhs.bindGroupLayoutMetaInfos[i].textures[j].index ||
                lhs.bindGroupLayoutMetaInfos[i].textures[j].stages != rhs.bindGroupLayoutMetaInfos[i].textures[j].stages)
            {
                return false;
            }
        }
    }

    return true;
}

} // namespace jipu