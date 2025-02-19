#pragma once

#include "adreno_sampler.h"

namespace hpc
{
namespace backend
{
namespace adreno
{

class AdrenoSamplerA6XX final : public AdrenoSampler
{
public:
    explicit AdrenoSamplerA6XX(const AdrenoGPU& gpu, std::unique_ptr<Handle> handle, const SamplerDescriptor& descriptor);
    ~AdrenoSamplerA6XX() override = default;

protected:
    uint32_t getGroup(uint32_t counter) override;
    uint32_t getSelector(Counter counter) override;
    Counter getCounter(uint32_t group, uint32_t selector) override;
};

} // namespace adreno
} // namespace backend
} // namespace hpc