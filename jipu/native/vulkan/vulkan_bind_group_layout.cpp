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
    const uint64_t storageTextureSize = descriptor.storageTextures.size();

    vkdescriptor.buffers.resize(bufferSize);
    vkdescriptor.samplers.resize(samplerSize);
    vkdescriptor.textures.resize(textureSize + storageTextureSize);

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

    for (uint64_t i = textureSize; i < storageTextureSize + textureSize; ++i)
    {
        const auto& storageTexture = descriptor.storageTextures[i];
        vkdescriptor.textures[i] = { .binding = storageTexture.index,
                                     .descriptorType = ToVkDescriptorType(storageTexture.access),
                                     .descriptorCount = 1,
                                     .stageFlags = ToVkShaderStageFlags(storageTexture.stages),
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
    , m_descriptor(descriptor)
    , m_vkdescriptor(generateVulkanBindGroupLayoutDescriptor(descriptor))
{
    m_info = {
        .buffers = getBufferBindingLayouts(),
        .samplers = getSamplerBindingLayouts(),
        .textures = getTextureBindingLayouts(),
        .storageTextures = getStorageTextureBindingLayouts(),
    };
}

VulkanBindGroupLayout::~VulkanBindGroupLayout()
{
    // do not destroy descriptor set layout here. because it is managed by cache.
}

std::vector<BufferBindingLayout> VulkanBindGroupLayout::getBufferBindingLayouts() const
{
    return m_descriptor.buffers;
}

std::vector<SamplerBindingLayout> VulkanBindGroupLayout::getSamplerBindingLayouts() const
{
    return m_descriptor.samplers;
}

std::vector<TextureBindingLayout> VulkanBindGroupLayout::getTextureBindingLayouts() const
{
    return m_descriptor.textures;
}

std::vector<StorageTextureBindingLayout> VulkanBindGroupLayout::getStorageTextureBindingLayouts() const
{
    return m_descriptor.storageTextures;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getDescriptorSetLayouts() const
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{};

    bindings.insert(bindings.end(), m_vkdescriptor.buffers.begin(), m_vkdescriptor.buffers.end());
    bindings.insert(bindings.end(), m_vkdescriptor.samplers.begin(), m_vkdescriptor.samplers.end());
    bindings.insert(bindings.end(), m_vkdescriptor.textures.begin(), m_vkdescriptor.textures.end());

    return bindings;
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getBufferDescriptorSetLayouts() const
{
    return m_vkdescriptor.buffers;
}

VkDescriptorSetLayoutBinding VulkanBindGroupLayout::getBufferDescriptorSetLayout(uint32_t index) const
{
    if (m_vkdescriptor.buffers.empty())
        throw std::runtime_error("Failed to find buffer binding layout due to empty.");

    if (m_vkdescriptor.buffers.size() <= index)
        throw std::runtime_error("Failed to find buffer binding layout due to over index.");

    return m_vkdescriptor.buffers[index];
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getSamplerDescriptorSetLayouts() const
{
    return m_vkdescriptor.samplers;
}

VkDescriptorSetLayoutBinding VulkanBindGroupLayout::getSamplerDescriptorSetLayout(uint32_t index) const
{
    if (m_vkdescriptor.samplers.empty())
        throw std::runtime_error("Failed to find sampler binding layout due to empty.");

    if (m_vkdescriptor.samplers.size() <= index)
        throw std::runtime_error("Failed to find sampler binding layout due to over index.");

    return m_vkdescriptor.samplers[index];
}

std::vector<VkDescriptorSetLayoutBinding> VulkanBindGroupLayout::getTextureDescriptorSetLayouts() const
{
    return m_vkdescriptor.textures;
}

VkDescriptorSetLayoutBinding VulkanBindGroupLayout::getTextureDescriptorSetLayout(uint32_t index) const
{
    if (m_vkdescriptor.textures.empty())
        throw std::runtime_error("Failed to find texture binding layout due to empty.");

    if (m_vkdescriptor.textures.size() <= index)
        throw std::runtime_error("Failed to find texture binding layout due to over index.");

    return m_vkdescriptor.textures[index];
}

VkDescriptorSetLayout VulkanBindGroupLayout::getVkDescriptorSetLayout() const
{
    VulkanBindGroupLayoutMetaData metaData{
        .info = m_info,
    };
    return m_device->getBindGroupLayoutCache()->getVkDescriptorSetLayout(metaData);
}

const VulkanBindGroupLayoutInfo& VulkanBindGroupLayout::getInfo() const
{
    return m_info;
}

// VulkanBindGroupLayoutCache

size_t VulkanBindGroupLayoutCache::Functor::operator()(const VulkanBindGroupLayoutMetaData& metaData) const
{
    size_t hash = 0;

    for (const auto& buffer : metaData.info.buffers)
    {
        combineHash(hash, buffer.dynamicOffset);
        combineHash(hash, buffer.index);
        combineHash(hash, buffer.stages);
        combineHash(hash, buffer.type);
    }

    for (const auto& sampler : metaData.info.samplers)
    {
        combineHash(hash, sampler.index);
        combineHash(hash, sampler.stages);
    }

    for (const auto& texture : metaData.info.textures)
    {
        combineHash(hash, texture.index);
        combineHash(hash, texture.stages);
    }

    for (const auto& storageTexture : metaData.info.storageTextures)
    {
        combineHash(hash, storageTexture.index);
        combineHash(hash, storageTexture.stages);
        combineHash(hash, storageTexture.access);
    }

    return hash;
}

bool VulkanBindGroupLayoutCache::Functor::operator()(const VulkanBindGroupLayoutMetaData& lhs,
                                                     const VulkanBindGroupLayoutMetaData& rhs) const
{
    if (lhs.info.buffers.size() != rhs.info.buffers.size() ||
        lhs.info.samplers.size() != rhs.info.samplers.size() ||
        lhs.info.textures.size() != rhs.info.textures.size() ||
        lhs.info.storageTextures.size() != rhs.info.storageTextures.size())
    {
        return false;
    }

    for (auto i = 0; i < lhs.info.buffers.size(); ++i)
    {
        if (lhs.info.buffers[i].dynamicOffset != rhs.info.buffers[i].dynamicOffset ||
            lhs.info.buffers[i].index != rhs.info.buffers[i].index ||
            lhs.info.buffers[i].stages != rhs.info.buffers[i].stages ||
            lhs.info.buffers[i].type != rhs.info.buffers[i].type)
        {
            return false;
        }
    }

    for (auto i = 0; i < lhs.info.samplers.size(); ++i)
    {
        if (lhs.info.samplers[i].index != rhs.info.samplers[i].index ||
            lhs.info.samplers[i].stages != rhs.info.samplers[i].stages)
        {
            return false;
        }
    }

    for (auto i = 0; i < lhs.info.textures.size(); ++i)
    {
        if (lhs.info.textures[i].index != rhs.info.textures[i].index ||
            lhs.info.textures[i].stages != rhs.info.textures[i].stages)
        {
            return false;
        }
    }

    for (auto i = 0; i < lhs.info.storageTextures.size(); ++i)
    {
        if (lhs.info.storageTextures[i].index != rhs.info.storageTextures[i].index ||
            lhs.info.storageTextures[i].stages != rhs.info.storageTextures[i].stages ||
            lhs.info.storageTextures[i].access != rhs.info.storageTextures[i].access)
        {
            return false;
        }
    }

    return true;
}

VulkanBindGroupLayoutCache::VulkanBindGroupLayoutCache(VulkanDevice* device)
    : m_device(device)
{
}

VulkanBindGroupLayoutCache::~VulkanBindGroupLayoutCache()
{
    clear();
}

VkDescriptorSetLayout VulkanBindGroupLayoutCache::getVkDescriptorSetLayout(const VulkanBindGroupLayoutMetaData& metaData)
{
    auto it = m_bindGroupLayouts.find(metaData);
    if (it != m_bindGroupLayouts.end())
    {
        return it->second;
    }

    BindGroupLayoutDescriptor descriptor{};
    descriptor.buffers = metaData.info.buffers;
    descriptor.samplers = metaData.info.samplers;
    descriptor.textures = metaData.info.textures;
    descriptor.storageTextures = metaData.info.storageTextures;

    VkDescriptorSetLayout layout = createDescriptorSetLayout(m_device, generateVulkanBindGroupLayoutDescriptor(descriptor));

    m_bindGroupLayouts.insert({ metaData, layout });

    return layout;
}

void VulkanBindGroupLayoutCache::clear()
{
    for (auto& [_, layout] : m_bindGroupLayouts)
    {
        m_device->getDeleter()->safeDestroy(layout);
    }

    m_bindGroupLayouts.clear();
}

// Convert Helper

VkDescriptorType ToVkDescriptorType(StorageTextureAccess access)
{
    switch (access)
    {
    case StorageTextureAccess::kReadOnly:
    case StorageTextureAccess::kWriteOnly:
    case StorageTextureAccess::kReadWrite:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    default:
    case StorageTextureAccess::kUndefined:
        throw std::runtime_error(fmt::format("Failed to support access [{}] for VkDescriptorType.", static_cast<int32_t>(access)));
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }
}
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
    case BufferBindingType::kReadOnlyStorage:
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