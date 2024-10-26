#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace jipu
{

struct ComputePassEncoderDescriptor
{
};

class ComputePipeline;
class BindingGroup;
class ComputePassEncoder
{
public:
    virtual ~ComputePassEncoder() = default;

    virtual void setPipeline(ComputePipeline& pipeline) = 0;
    virtual void setBindingGroup(uint32_t index, BindingGroup& bindingGroup, std::vector<uint32_t> dynamicOffset = {}) = 0;
    virtual void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) = 0;
    virtual void end() = 0;
};

} // namespace jipu