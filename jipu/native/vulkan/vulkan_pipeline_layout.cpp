#include "vulkan_pipeline_layout.h"
#include "vulkan_binding_group_layout.h"
#include "vulkan_device.h"

#include <stdexcept>

namespace jipu
{

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice& device, const PipelineLayoutDescriptor& descriptor)
    : m_device(device)
{
    std::vector<VkDescriptorSetLayout> layouts{};
    layouts.resize(descriptor.layouts.size());
    for (uint32_t i = 0; i < descriptor.layouts.size(); ++i)
    {
        layouts[i] = downcast(descriptor.layouts[i])->getVkDescriptorSetLayout();
    }

    VkPipelineLayoutCreateInfo createInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                           .setLayoutCount = static_cast<uint32_t>(layouts.size()),
                                           .pSetLayouts = layouts.data() };

    VkResult result = device.vkAPI.CreatePipelineLayout(device.getVkDevice(), &createInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkPipelineLayout");
    }
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    auto& vulkanDevice = downcast(m_device);
    vulkanDevice.vkAPI.DestroyPipelineLayout(vulkanDevice.getVkDevice(), m_pipelineLayout, nullptr);
}

VkPipelineLayout VulkanPipelineLayout::getVkPipelineLayout() const
{
    return m_pipelineLayout;
}

} // namespace jipu