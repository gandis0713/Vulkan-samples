#include "webgpu_sampler.h"
#include "webgpu_device.h"

#include "jipu/native/sampler.h"

namespace jipu
{

WebGPUSampler* WebGPUSampler::create(WebGPUDevice* device, WGPUSamplerDescriptor const* descriptor)
{
    WGPUSamplerDescriptor wgpuDescriptor = descriptor ? *descriptor : GenerateWGPUSamplerDescriptor();

    SamplerDescriptor samplerDescriptor{};
    samplerDescriptor.addressModeU = ToAddressMode(wgpuDescriptor.addressModeU);
    samplerDescriptor.addressModeV = ToAddressMode(wgpuDescriptor.addressModeV);
    samplerDescriptor.addressModeW = ToAddressMode(wgpuDescriptor.addressModeW);
    samplerDescriptor.magFilter = ToFilterMode(wgpuDescriptor.magFilter);
    samplerDescriptor.minFilter = ToFilterMode(wgpuDescriptor.minFilter);
    samplerDescriptor.mipmapFilter = ToMipmapFilterMode(wgpuDescriptor.mipmapFilter);
    samplerDescriptor.lodMin = wgpuDescriptor.lodMinClamp;
    samplerDescriptor.lodMax = wgpuDescriptor.lodMaxClamp;

    auto sampler = device->getDevice()->createSampler(samplerDescriptor);

    return new WebGPUSampler(device, std::move(sampler), &wgpuDescriptor);
}

WebGPUSampler::WebGPUSampler(WebGPUDevice* device, std::unique_ptr<Sampler> sampler, WGPUSamplerDescriptor const* descriptor)
    : m_wgpuDevice(device)
    , m_descriptor(*descriptor)
    , m_sampler(std::move(sampler))
{
}

Sampler* WebGPUSampler::getSampler() const
{
    return m_sampler.get();
}

// Generators
WGPUSamplerDescriptor GenerateWGPUSamplerDescriptor()
{
    WGPUSamplerDescriptor descriptor{};
    descriptor.addressModeU = WGPUAddressMode::WGPUAddressMode_ClampToEdge;
    descriptor.addressModeV = WGPUAddressMode::WGPUAddressMode_ClampToEdge;
    descriptor.addressModeW = WGPUAddressMode::WGPUAddressMode_ClampToEdge;
    descriptor.magFilter = WGPUFilterMode::WGPUFilterMode_Linear;
    descriptor.minFilter = WGPUFilterMode::WGPUFilterMode_Linear;
    descriptor.mipmapFilter = WGPUMipmapFilterMode::WGPUMipmapFilterMode_Linear;
    descriptor.lodMinClamp = 0.0f;
    descriptor.lodMaxClamp = 32.0f;
    descriptor.compare = WGPUCompareFunction::WGPUCompareFunction_Never;
    descriptor.maxAnisotropy = 1;

    return descriptor;
}

// Convert from JIPU to WebGPU
WGPUAddressMode ToWGPUAddressMode(AddressMode mode)
{
    switch (mode)
    {
    case AddressMode::kClampToEdge:
        return WGPUAddressMode::WGPUAddressMode_ClampToEdge;
    case AddressMode::kRepeat:
        return WGPUAddressMode::WGPUAddressMode_Repeat;
    case AddressMode::kMirrorRepeat:
        return WGPUAddressMode::WGPUAddressMode_MirrorRepeat;
    default:
        return WGPUAddressMode::WGPUAddressMode_ClampToEdge;
    }
}

WGPUFilterMode ToWGPUFilterMode(FilterMode mode)
{
    switch (mode)
    {
    case FilterMode::kNearest:
        return WGPUFilterMode::WGPUFilterMode_Nearest;
    case FilterMode::kLinear:
        return WGPUFilterMode::WGPUFilterMode_Linear;
    default:
        return WGPUFilterMode::WGPUFilterMode_Linear;
    }
}

WGPUMipmapFilterMode ToWGPUMipmapFilterMode(MipmapFilterMode mode)
{
    switch (mode)
    {
    case MipmapFilterMode::kNearest:
        return WGPUMipmapFilterMode::WGPUMipmapFilterMode_Nearest;
    case MipmapFilterMode::kLinear:
        return WGPUMipmapFilterMode::WGPUMipmapFilterMode_Linear;
    default:
        return WGPUMipmapFilterMode::WGPUMipmapFilterMode_Linear;
    }
}

// Convert from WebGPU to JIPU
AddressMode ToAddressMode(WGPUAddressMode mode)
{
    switch (mode)
    {
    case WGPUAddressMode::WGPUAddressMode_ClampToEdge:
        return AddressMode::kClampToEdge;
    case WGPUAddressMode::WGPUAddressMode_Repeat:
        return AddressMode::kRepeat;
    case WGPUAddressMode::WGPUAddressMode_MirrorRepeat:
        return AddressMode::kMirrorRepeat;
    default:
        return AddressMode::kClampToEdge;
    }
}

FilterMode ToFilterMode(WGPUFilterMode mode)
{
    switch (mode)
    {
    case WGPUFilterMode::WGPUFilterMode_Nearest:
        return FilterMode::kNearest;
    case WGPUFilterMode::WGPUFilterMode_Linear:
        return FilterMode::kLinear;
    default:
        return FilterMode::kLinear;
    }
}

MipmapFilterMode ToMipmapFilterMode(WGPUMipmapFilterMode mode)
{
    switch (mode)
    {
    case WGPUMipmapFilterMode::WGPUMipmapFilterMode_Nearest:
        return MipmapFilterMode::kNearest;
    case WGPUMipmapFilterMode::WGPUMipmapFilterMode_Linear:
        return MipmapFilterMode::kLinear;
    default:
        return MipmapFilterMode::kLinear;
    }
}

} // namespace jipu