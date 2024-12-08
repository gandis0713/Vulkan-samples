
#pragma once

#include "jipu/common/ref_counted.h"
#include "jipu/native/sampler.h"
#include "jipu/webgpu/webgpu_header.h"

#include <memory>

namespace jipu
{

class WebGPUDevice;
class WebGPUSampler : public RefCounted
{

public:
    static WebGPUSampler* create(WebGPUDevice* device, WGPUSamplerDescriptor const* descriptor);

public:
    WebGPUSampler() = delete;
    explicit WebGPUSampler(WebGPUDevice* device, std::unique_ptr<Sampler> sampler, WGPUSamplerDescriptor const* descriptor);

public:
    virtual ~WebGPUSampler() = default;

    WebGPUSampler(const WebGPUSampler&) = delete;
    WebGPUSampler& operator=(const WebGPUSampler&) = delete;

public: // WebGPU API
public:
    Sampler* getSampler() const;

private:
    [[maybe_unused]] WebGPUDevice* m_wgpuDevice = nullptr;
    [[maybe_unused]] const WGPUSamplerDescriptor m_descriptor{};

private:
    std::unique_ptr<Sampler> m_sampler = nullptr;
};

// Generators
WGPUSamplerDescriptor GenerateWGPUSamplerDescriptor();

// Convert from JIPU to WebGPU
WGPUAddressMode ToWGPUAddressMode(AddressMode mode);
WGPUFilterMode ToWGPUFilterMode(FilterMode mode);
WGPUMipmapFilterMode ToWGPUMipmapFilterMode(MipmapFilterMode mode);

// Convert from WebGPU to JIPU
AddressMode ToAddressMode(WGPUAddressMode mode);
FilterMode ToFilterMode(WGPUFilterMode mode);
MipmapFilterMode ToMipmapFilterMode(WGPUMipmapFilterMode mode);

} // namespace jipu