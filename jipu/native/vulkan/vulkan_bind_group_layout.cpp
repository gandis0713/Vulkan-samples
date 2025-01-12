#include "vulkan_bind_group_layout.h"
#include "vulkan_device.h"

#include <fmt/format.h>
#include <stdexcept>

namespace jipu
{

VulkanBindGroupLayoutDescriptor generateVulkanBindGroupLayoutDescriptor(const BindGroupLayoutDescriptor& descriptor)
{
    VulkanBindGroupLayoutDescriptor vkdescriptor{};

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

VulkanBindGroupLayout::VulkanBindGroupLayout(VulkanDevice* device, const BindGroupLayoutDescriptor& descriptor)
    : VulkanBindGroupLayout(device, generateVulkanBindGroupLayoutDescriptor(descriptor))
{
}

VulkanBindGroupLayout::VulkanBindGroupLayout(VulkanDevice* device, const VulkanBindGroupLayoutDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    // TODO: use cached layout

    std::vector<VkDescriptorSetLayoutBinding> bindings{};
    bindings.insert(bindings.end(), m_descriptor.buffers.begin(), m_descriptor.buffers.end());
    bindings.insert(bindings.end(), m_descriptor.samplers.begin(), m_descriptor.samplers.end());
    bindings.insert(bindings.end(), m_descriptor.textures.begin(), m_descriptor.textures.end());

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                      .bindingCount = static_cast<uint32_t>(bindings.size()),
                                                      .pBindings = bindings.data() };

    const VulkanAPI& vkAPI = device->vkAPI;
    VkResult result = vkAPI.CreateDescriptorSetLayout(device->getVkDevice(), &layoutCreateInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkDescriptorSetLayout");
    }
}

VulkanBindGroupLayout::~VulkanBindGroupLayout()
{
    m_device->getDeleter()->safeDestroy(m_descriptorSetLayout);
}

std::vector<BufferBindingLayout> VulkanBindGroupLayout::getBufferBindingLayouts() const
{
    std::vector<BufferBindingLayout> layouts{};
    for (const auto& buffer : m_descriptor.buffers)
    {
        layouts.push_back({ .index = buffer.binding,
                            .stages = ToBindingStageFlags(buffer.stageFlags),
                            .type = ToBufferBindingType(buffer.descriptorType),
                            .dynamicOffset = buffer.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                                             buffer.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC });
    }

    return layouts;
}

std::vector<SamplerBindingLayout> VulkanBindGroupLayout::getSamplerBindingLayouts() const
{
    std::vector<SamplerBindingLayout> layouts{};
    for (const auto& sampler : m_descriptor.samplers)
    {
        layouts.push_back({ .index = sampler.binding,
                            .stages = ToBindingStageFlags(sampler.stageFlags) });
    }

    return layouts;
}

std::vector<TextureBindingLayout> VulkanBindGroupLayout::getTextureBindingLayouts() const
{
    std::vector<TextureBindingLayout> layouts{};
    for (const auto& texture : m_descriptor.textures)
    {
        layouts.push_back({ .index = texture.binding,
                            .stages = ToBindingStageFlags(texture.stageFlags) });
    }

    return layouts;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getDescriptorSetLayouts() const
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{};

    bindings.insert(bindings.end(), m_descriptor.buffers.begin(), m_descriptor.buffers.end());
    bindings.insert(bindings.end(), m_descriptor.samplers.begin(), m_descriptor.samplers.end());
    bindings.insert(bindings.end(), m_descriptor.textures.begin(), m_descriptor.textures.end());

    return bindings;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getBufferDescriptorSetLayouts() const
{
    return m_descriptor.buffers;
}

VkDescriptorSetLayoutBinding VulkanBindGroupLayout::getBufferDescriptorSetLayout(uint32_t index) const
{
    if (m_descriptor.buffers.empty())
        throw std::runtime_error("Failed to find buffer binding layout due to empty.");

    if (m_descriptor.buffers.size() <= index)
        throw std::runtime_error("Failed to find buffer binding layout due to over index.");

    return m_descriptor.buffers[index];
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getSamplerDescriptorSetLayouts() const
{
    return m_descriptor.samplers;
}

VkDescriptorSetLayoutBinding VulkanBindGroupLayout::getSamplerDescriptorSetLayout(uint32_t index) const
{
    if (m_descriptor.samplers.empty())
        throw std::runtime_error("Failed to find sampler binding layout due to empty.");

    if (m_descriptor.samplers.size() <= index)
        throw std::runtime_error("Failed to find sampler binding layout due to over index.");

    return m_descriptor.samplers[index];
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getTextureDescriptorSetLayouts() const
{
    return m_descriptor.textures;
}

VkDescriptorSetLayoutBinding VulkanBindGroupLayout::getTextureDescriptorSetLayout(uint32_t index) const
{
    if (m_descriptor.textures.empty())
        throw std::runtime_error("Failed to find texture binding layout due to empty.");

    if (m_descriptor.textures.size() <= index)
        throw std::runtime_error("Failed to find texture binding layout due to over index.");

    return m_descriptor.textures[index];
}

VkDescriptorSetLayout VulkanBindGroupLayout::getVkDescriptorSetLayout() const
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

    if (vkFlags & VK_SHADER_STAGE_VERTEX_BIT)
    {
        flags |= BindingStageFlagBits::kVertexStage;
    }
    if (vkFlags & VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        flags |= BindingStageFlagBits::kFragmentStage;
    }
    if (vkFlags & VK_SHADER_STAGE_COMPUTE_BIT)
    {
        flags |= BindingStageFlagBits::kComputeStage;
    }

    return flags;
}

} // namespace jipu