#include "vulkan_bind_group_layout.h"
#include "vulkan_device.h"

#include "jipu/common/hash.h"
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

VkDescriptorSetLayout createDescriptorSetLayout(VulkanDevice* device, const VulkanBindGroupLayoutDescriptor& descriptor)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{};
    bindings.insert(bindings.end(), descriptor.buffers.begin(), descriptor.buffers.end());
    bindings.insert(bindings.end(), descriptor.samplers.begin(), descriptor.samplers.end());
    bindings.insert(bindings.end(), descriptor.textures.begin(), descriptor.textures.end());

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                      .bindingCount = static_cast<uint32_t>(bindings.size()),
                                                      .pBindings = bindings.data() };

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    const VulkanAPI& vkAPI = device->vkAPI;
    VkResult result = vkAPI.CreateDescriptorSetLayout(device->getVkDevice(), &layoutCreateInfo, nullptr, &descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkDescriptorSetLayout");
    }

    return descriptorSetLayout;
}

VulkanBindGroupLayout::VulkanBindGroupLayout(VulkanDevice* device, const BindGroupLayoutDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(generateVulkanBindGroupLayoutDescriptor(descriptor))
{
    m_descriptorSetLayout = m_device->getBindGroupLayoutCache()->getVkDescriptorSetLayout(descriptor);
}

VulkanBindGroupLayout::~VulkanBindGroupLayout()
{
    // do not destroy descriptor set layout here. because it is managed by cache.
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

BindGroupLayoutMetaInfo VulkanBindGroupLayout::getLayoutInfo() const
{
    BindGroupLayoutMetaInfo layoutInfo{};
    layoutInfo.buffers = getBufferBindingLayouts();
    layoutInfo.samplers = getSamplerBindingLayouts();
    layoutInfo.textures = getTextureBindingLayouts();

    return layoutInfo;
}

// VulkanBindGroupLayoutCache

size_t VulkanBindGroupLayoutCache::getHash(const BindGroupLayoutMetaInfo& layoutInfo)
{
    size_t hash = 0;

    for (const auto& buffer : layoutInfo.buffers)
    {
        combineHash(hash, buffer.dynamicOffset);
        combineHash(hash, buffer.index);
        combineHash(hash, buffer.stages);
        combineHash(hash, buffer.type);
    }

    for (const auto& sampler : layoutInfo.samplers)
    {
        combineHash(hash, sampler.index);
        combineHash(hash, sampler.stages);
    }

    for (const auto& texture : layoutInfo.textures)
    {
        combineHash(hash, texture.index);
        combineHash(hash, texture.stages);
    }

    return hash;
}

VulkanBindGroupLayoutCache::VulkanBindGroupLayoutCache(VulkanDevice* device)
    : m_device(device)
{
}

VulkanBindGroupLayoutCache::~VulkanBindGroupLayoutCache()
{
    clear();
}

VkDescriptorSetLayout VulkanBindGroupLayoutCache::getVkDescriptorSetLayout(const BindGroupLayoutDescriptor& descriptor)
{
    return getVkDescriptorSetLayout(BindGroupLayoutMetaInfo{
        .buffers = descriptor.buffers,
        .samplers = descriptor.samplers,
        .textures = descriptor.textures,
    });
}

VkDescriptorSetLayout VulkanBindGroupLayoutCache::getVkDescriptorSetLayout(const BindGroupLayoutMetaInfo& layoutInfo)
{
    auto it = m_bindGroupLayouts.find(layoutInfo);
    if (it != m_bindGroupLayouts.end())
    {
        return it->second;
    }

    BindGroupLayoutDescriptor descriptor{};
    descriptor.buffers = layoutInfo.buffers;
    descriptor.samplers = layoutInfo.samplers;
    descriptor.textures = layoutInfo.textures;

    VkDescriptorSetLayout layout = createDescriptorSetLayout(m_device, generateVulkanBindGroupLayoutDescriptor(descriptor));

    m_bindGroupLayouts.insert({ layoutInfo, layout });

    return layout;
}

void VulkanBindGroupLayoutCache::clear()
{
    for (auto& [descriptor, layout] : m_bindGroupLayouts)
    {
        m_device->getDeleter()->safeDestroy(layout);
    }

    m_bindGroupLayouts.clear();
}

size_t VulkanBindGroupLayoutCache::Functor::operator()(const BindGroupLayoutMetaInfo& layoutInfo) const
{
    return getHash(layoutInfo);
}

bool VulkanBindGroupLayoutCache::Functor::operator()(const BindGroupLayoutMetaInfo& lhs,
                                                     const BindGroupLayoutMetaInfo& rhs) const
{
    if (lhs.buffers.size() != rhs.buffers.size() ||
        lhs.samplers.size() != rhs.samplers.size() ||
        lhs.textures.size() != rhs.textures.size())
    {
        return false;
    }

    for (auto j = 0; j < lhs.buffers.size(); ++j)
    {
        if (lhs.buffers[j].dynamicOffset != rhs.buffers[j].dynamicOffset ||
            lhs.buffers[j].index != rhs.buffers[j].index ||
            lhs.buffers[j].stages != rhs.buffers[j].stages ||
            lhs.buffers[j].type != rhs.buffers[j].type)
        {
            return false;
        }
    }

    for (auto j = 0; j < lhs.samplers.size(); ++j)
    {
        if (lhs.samplers[j].index != rhs.samplers[j].index ||
            lhs.samplers[j].stages != rhs.samplers[j].stages)
        {
            return false;
        }
    }

    for (auto j = 0; j < lhs.textures.size(); ++j)
    {
        if (lhs.textures[j].index != rhs.textures[j].index ||
            lhs.textures[j].stages != rhs.textures[j].stages)
        {
            return false;
        }
    }

    return true;
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