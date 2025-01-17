#include "webgpu_bind_group_layout.h"

#include "webgpu_device.h"

namespace jipu
{

WebGPUBindGroupLayout* WebGPUBindGroupLayout::create(WebGPUDevice* wgpuDevice, WGPUBindGroupLayoutDescriptor const* descriptor)
{
    BindGroupLayoutDescriptor layoutDescriptor{};
    for (auto i = 0; i < descriptor->entryCount; i++)
    {
        auto entry = descriptor->entries[i];
        if (entry.buffer.type)
        {
            layoutDescriptor.buffers.push_back(BufferBindingLayout{
                .index = entry.binding,
                .stages = WGPUToBindingStageFlags(entry.visibility),
                .type = WGPUToBufferBindingType(entry.buffer.type),
                .dynamicOffset = static_cast<bool>(entry.buffer.hasDynamicOffset),
            });
        }

        if (entry.sampler.type)
        {
            // TODO: set sampler type
            layoutDescriptor.samplers.push_back(SamplerBindingLayout{
                .index = entry.binding,
                .stages = WGPUToBindingStageFlags(entry.visibility),
            });
        }

        if (entry.texture.sampleType)
        {
            // TODO: set texture sample type and dimension and multisampled
            layoutDescriptor.textures.push_back(TextureBindingLayout{
                .index = entry.binding,
                .stages = WGPUToBindingStageFlags(entry.visibility),
            });
        }

        if (entry.storageTexture.access)
        {
            layoutDescriptor.storageTextures.push_back(StorageTextureBindingLayout{
                .index = entry.binding,
                .stages = WGPUToBindingStageFlags(entry.visibility),
                .type = WGPUToStorageTextureBindingType(entry.storageTexture.access),
            });
        }
    }

    auto device = wgpuDevice->getDevice();
    auto layout = device->createBindGroupLayout(layoutDescriptor);

    return new WebGPUBindGroupLayout(wgpuDevice, std::move(layout), descriptor);
}

WebGPUBindGroupLayout::WebGPUBindGroupLayout(WebGPUDevice* wgpuDevice, std::unique_ptr<BindGroupLayout> layout, WGPUBindGroupLayoutDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_layout(std::move(layout))
{
}

BindGroupLayout* WebGPUBindGroupLayout::getBindGroupLayout() const
{
    return m_layout.get();
}

// Convert from JIPU to WebGPU
WGPUShaderStage ToWGPUShaderStage(BindingStageFlags stages)
{
    WGPUShaderStage wgpuStages = WGPUShaderStage_None;
    if (stages & BindingStageFlagBits::kVertexStage)
    {
        wgpuStages |= WGPUShaderStage_Vertex;
    }
    if (stages & BindingStageFlagBits::kFragmentStage)
    {
        wgpuStages |= WGPUShaderStage_Fragment;
    }
    if (stages & BindingStageFlagBits::kComputeStage)
    {
        wgpuStages |= WGPUShaderStage_Compute;
    }
    return wgpuStages;
}

WGPUBufferBindingType ToWGPUBufferBindingType(BufferBindingType type)
{
    switch (type)
    {
    case BufferBindingType::kUniform:
        return WGPUBufferBindingType_Uniform;
    case BufferBindingType::kStorage:
        return WGPUBufferBindingType_Storage;
    case BufferBindingType::kReadOnlyStorage:
        return WGPUBufferBindingType_ReadOnlyStorage;
    default:
        return WGPUBufferBindingType_Undefined;
    }
}

WGPUStorageTextureAccess ToWGPUStorageTextureAccess(StorageTextureBindingType type)
{
    switch (type)
    {
    case StorageTextureBindingType::kWriteOnly:
        return WGPUStorageTextureAccess_WriteOnly;
    case StorageTextureBindingType::kReadOnly:
        return WGPUStorageTextureAccess_ReadOnly;
    case StorageTextureBindingType::kReadWrite:
        return WGPUStorageTextureAccess_ReadWrite;
    default:
        return WGPUStorageTextureAccess_Undefined;
    }
}

// Convert from WebGPU to JIPU
BindingStageFlags WGPUToBindingStageFlags(WGPUShaderStage stages)
{
    BindingStageFlags flags = BindingStageFlagBits::kUndefined;
    if (stages & WGPUShaderStage_Vertex)
    {
        flags |= BindingStageFlagBits::kVertexStage;
    }
    if (stages & WGPUShaderStage_Fragment)
    {
        flags |= BindingStageFlagBits::kFragmentStage;
    }
    if (stages & WGPUShaderStage_Compute)
    {
        flags |= BindingStageFlagBits::kComputeStage;
    }
    return flags;
}

BufferBindingType WGPUToBufferBindingType(WGPUBufferBindingType type)
{
    switch (type)
    {
    case WGPUBufferBindingType_Uniform:
        return BufferBindingType::kUniform;
    case WGPUBufferBindingType_Storage:
        return BufferBindingType::kStorage;
    case WGPUBufferBindingType_ReadOnlyStorage:
        return BufferBindingType::kReadOnlyStorage;
    default:
        return BufferBindingType::kUndefined;
    }
}

StorageTextureBindingType WGPUToStorageTextureBindingType(WGPUStorageTextureAccess access)
{
    switch (access)
    {
    case WGPUStorageTextureAccess_WriteOnly:
        return StorageTextureBindingType::kWriteOnly;
    case WGPUStorageTextureAccess_ReadOnly:
        return StorageTextureBindingType::kReadOnly;
    case WGPUStorageTextureAccess_ReadWrite:
        return StorageTextureBindingType::kReadWrite;
    default:
        return StorageTextureBindingType::kUndefined;
    }
}

} // namespace jipu