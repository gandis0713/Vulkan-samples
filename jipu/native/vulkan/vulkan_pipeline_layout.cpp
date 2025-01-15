#include "vulkan_pipeline_layout.h"

#include "vulkan_bind_group_layout.h"
#include "vulkan_device.h"

#include "jipu/common/hash.h"
#include <stdexcept>

namespace jipu
{

static size_t getHash(const VulkanPipelineLayoutInfo& layoutInfo)
{
    size_t hash = 0;

    // use hash instead of layout info
    for (const auto& bindGroupLayoutMetaData : layoutInfo.bindGroupLayoutMetaDatas)
    {
        combineHash(hash, bindGroupLayoutMetaData.hash);
    }

    // for (const auto& bindGroupLayoutMetaData : layoutInfo.bindGroupLayoutMetaDatas)
    // {
    //     const auto& bindGroupLayoutInfo = bindGroupLayoutMetaData.info;
    //     for (const auto& buffer : bindGroupLayoutInfo.buffers)
    //     {
    //         combineHash(hash, buffer.dynamicOffset);
    //         combineHash(hash, buffer.index);
    //         combineHash(hash, buffer.stages);
    //         combineHash(hash, buffer.type);
    //     }

    //     for (const auto& sampler : bindGroupLayoutInfo.samplers)
    //     {
    //         combineHash(hash, sampler.index);
    //         combineHash(hash, sampler.stages);
    //     }

    //     for (const auto& texture : bindGroupLayoutInfo.textures)
    //     {
    //         combineHash(hash, texture.index);
    //         combineHash(hash, texture.stages);
    //     }
    // }

    return hash;
}

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device, const PipelineLayoutDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    m_layoutMetaData.info.bindGroupLayoutMetaDatas.resize(m_descriptor.layouts.size());

    for (uint32_t i = 0; i < m_layoutMetaData.info.bindGroupLayoutMetaDatas.size(); ++i)
    {
        auto bindGroupLayout = downcast(m_descriptor.layouts[i]);
        m_layoutMetaData.info.bindGroupLayoutMetaDatas[i] = bindGroupLayout->getMetaData();
    }

    m_layoutMetaData.hash = getHash(m_layoutMetaData.info);
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    // do not destroy VkPipelineLayout here. because it is managed by VulkanPipelineLayoutCache.
}

VkPipelineLayout VulkanPipelineLayout::getVkPipelineLayout() const
{
    return m_device->getPipelineLayoutCache()->getVkPipelineLayout(m_layoutMetaData);
}

const VulkanPipelineLayoutMetaData& VulkanPipelineLayout::getMetaData() const
{
    return m_layoutMetaData;
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

VkPipelineLayout VulkanPipelineLayoutCache::getVkPipelineLayout(const VulkanPipelineLayoutMetaData& layoutMetaData)
{
    auto it = m_pipelineLayouts.find(layoutMetaData.hash);
    if (it != m_pipelineLayouts.end())
    {
        return it->second;
    }

    std::vector<VkDescriptorSetLayout> layouts{};
    layouts.resize(layoutMetaData.info.bindGroupLayoutMetaDatas.size());
    for (uint32_t i = 0; i < layouts.size(); ++i)
    {
        layouts[i] = m_device->getBindGroupLayoutCache()->getVkDescriptorSetLayout(layoutMetaData.info.bindGroupLayoutMetaDatas[i]);
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

    m_pipelineLayouts.insert({ getHash(layoutMetaData.info), pipelineLayout });

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