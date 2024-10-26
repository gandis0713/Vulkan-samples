#include "vulkan_binding_group_layout.h"
#include "vulkan_device.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

VulkanBindingGroupLayoutDescriptor generateVulkanBindingGroupLayoutDescriptor(const BindingGroupLayoutDescriptor& descriptor)
{
    VulkanBindingGroupLayoutDescriptor vkdescriptor{};

    const uint64_t bufferSize = descriptor.buffers.size();
    const uint64_t samplerSize = descriptor.samplers.size();
    const uint64_t textureSize = descriptor.textures.size();

    vkdescriptor.buffers.resize(bufferSize);
    vkdescriptor.samplers.resize(samplerSize);
    vkdescriptor.textures.resize(textureSize);

    for (uint64_t i = 0; i < bufferSize; ++i)
    {
        const auto& buffer = descriptor.buffers[i];
        vkdescriptor.buffers[i] = { .binding = buffer.index,
                                    .descriptorType = ToVkDescriptorType(buffer.type, buffer.dynamicOffset),
                                    .descriptorCount = 1,
                                    .stageFlags = ToVkShaderStageFlags(buffer.stages),
                                    .pImmutableSamplers = nullptr };
    }

    for (uint64_t i = 0; i < samplerSize; ++i)
    {
        const auto& sampler = descriptor.samplers[i];
        vkdescriptor.samplers[i] = { .binding = sampler.index,
                                     .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                                     .descriptorCount = 1,
                                     .stageFlags = ToVkShaderStageFlags(sampler.stages),
                                     .pImmutableSamplers = nullptr };
    }

    for (uint64_t i = 0; i < textureSize; ++i)
    {
        const auto& texture = descriptor.textures[i];
        vkdescriptor.textures[i] = { .binding = texture.index,
                                     .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                     .descriptorCount = 1,
                                     .stageFlags = ToVkShaderStageFlags(texture.stages),
                                     .pImmutableSamplers = nullptr };
    }

    return vkdescriptor;
}

VulkanBindingGroupLayout::VulkanBindingGroupLayout(VulkanDevice& device, const BindingGroupLayoutDescriptor& descriptor)
    : VulkanBindingGroupLayout(device, generateVulkanBindingGroupLayoutDescriptor(descriptor))
{
}

VulkanBindingGroupLayout::VulkanBindingGroupLayout(VulkanDevice& device, const VulkanBindingGroupLayoutDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{};
    bindings.insert(bindings.end(), m_descriptor.buffers.begin(), m_descriptor.buffers.end());
    bindings.insert(bindings.end(), m_descriptor.samplers.begin(), m_descriptor.samplers.end());
    bindings.insert(bindings.end(), m_descriptor.textures.begin(), m_descriptor.textures.end());

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                      .bindingCount = static_cast<uint32_t>(bindings.size()),
                                                      .pBindings = bindings.data() };

    const VulkanAPI& vkAPI = device.vkAPI;
    VkResult result = vkAPI.CreateDescriptorSetLayout(device.getVkDevice(), &layoutCreateInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkDescriptorSetLayout");
    }
}

VulkanBindingGroupLayout::~VulkanBindingGroupLayout()
{
    auto& vulkanDevice = downcast(m_device);
    vulkanDevice.vkAPI.DestroyDescriptorSetLayout(vulkanDevice.getVkDevice(), m_descriptorSetLayout, nullptr);
}

std::vector<BufferBindingLayout> VulkanBindingGroupLayout::getBufferBindingLayouts() const
{
    std::vector<BufferBindingLayout> layouts{};
    for (const auto& buffer : m_descriptor.buffers)
    {
        layouts.push_back({ .index = buffer.binding,
                            .type = ToBufferBindingType(buffer.descriptorType),
                            .dynamicOffset = buffer.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                                             buffer.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
                            .stages = ToBindingStageFlags(buffer.stageFlags) });
    }

    return layouts;
}

std::vector<SamplerBindingLayout> VulkanBindingGroupLayout::getSamplerBindingLayouts() const
{
    std::vector<SamplerBindingLayout> layouts{};
    for (const auto& sampler : m_descriptor.samplers)
    {
        layouts.push_back({ .index = sampler.binding,
                            .stages = ToBindingStageFlags(sampler.stageFlags) });
    }

    return layouts;
}

std::vector<TextureBindingLayout> VulkanBindingGroupLayout::getTextureBindingLayouts() const
{
    std::vector<TextureBindingLayout> layouts{};
    for (const auto& texture : m_descriptor.textures)
    {
        layouts.push_back({ .index = texture.binding,
                            .stages = ToBindingStageFlags(texture.stageFlags) });
    }

    return layouts;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindingGroupLayout::getBufferDescriptorSetLayouts() const
{
    return m_descriptor.buffers;
}

VkDescriptorSetLayoutBinding VulkanBindingGroupLayout::getBufferDescriptorSetLayout(uint32_t index) const
{
    if (m_descriptor.buffers.empty())
        throw std::runtime_error("Failed to find buffer binding layout due to empty.");

    if (m_descriptor.buffers.size() <= index)
        throw std::runtime_error("Failed to find buffer binding layout due to over index.");

    return m_descriptor.buffers[index];
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindingGroupLayout::getSamplerDescriptorSetLayouts() const
{
    return m_descriptor.samplers;
}

VkDescriptorSetLayoutBinding VulkanBindingGroupLayout::getSamplerDescriptorSetLayout(uint32_t index) const
{
    if (m_descriptor.samplers.empty())
        throw std::runtime_error("Failed to find sampler binding layout due to empty.");

    if (m_descriptor.samplers.size() <= index)
        throw std::runtime_error("Failed to find sampler binding layout due to over index.");

    return m_descriptor.samplers[index];
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindingGroupLayout::getTextureDescriptorSetLayouts() const
{
    return m_descriptor.textures;
}

VkDescriptorSetLayoutBinding VulkanBindingGroupLayout::getTextureDescriptorSetLayout(uint32_t index) const
{
    if (m_descriptor.textures.empty())
        throw std::runtime_error("Failed to find texture binding layout due to empty.");

    if (m_descriptor.textures.size() <= index)
        throw std::runtime_error("Failed to find texture binding layout due to over index.");

    return m_descriptor.textures[index];
}

VkDescriptorSetLayout VulkanBindingGroupLayout::getVkDescriptorSetLayout() const
{
    return m_descriptorSetLayout;
}

// Convert Helper
VkDescriptorType ToVkDescriptorType(BufferBindingType type, bool dynamicOffset)
{
    switch (type)
    {
    case BufferBindingType::kUniform: {
        if (dynamicOffset)
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        else
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    case BufferBindingType::kStorage:
        if (dynamicOffset)
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        else
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    default:
    case BufferBindingType::kUndefined:
        throw std::runtime_error(fmt::format("Failed to support type [{}] for VkDescriptorType.", static_cast<int32_t>(type)));
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

BufferBindingType ToBufferBindingType(VkDescriptorType type)
{
    switch (type)
    {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return BufferBindingType::kUniform;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return BufferBindingType::kStorage;
    default:
        throw std::runtime_error(fmt::format("Failed to support type [{}] for BufferBindingType.", static_cast<int32_t>(type)));
        return BufferBindingType::kUndefined;
    }
}

VkShaderStageFlags ToVkShaderStageFlags(BindingStageFlags flags)
{
    VkShaderStageFlags vkFlags = 0x00000000; // 0x00000000

    if (flags & BindingStageFlagBits::kVertexStage)
    {
        vkFlags |= VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (flags & BindingStageFlagBits::kFragmentStage)
    {
        vkFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (flags & BindingStageFlagBits::kComputeStage)
    {
        vkFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }

    return vkFlags;
}

BindingStageFlags ToBindingStageFlags(VkShaderStageFlags vkFlags)
{
    BindingStageFlags flags = 0u;

    if (flags & VK_SHADER_STAGE_VERTEX_BIT)
    {
        vkFlags |= BindingStageFlagBits::kVertexStage;
    }
    if (flags & VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        vkFlags |= BindingStageFlagBits::kFragmentStage;
    }
    if (flags & VK_SHADER_STAGE_COMPUTE_BIT)
    {
        vkFlags |= BindingStageFlagBits::kComputeStage;
    }

    return flags;
}

} // namespace jipu