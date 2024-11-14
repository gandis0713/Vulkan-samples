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
class BindGroup;
class ComputePassEncoder
{
public:
    virtual ~ComputePassEncoder() = default;

    ComputePassEncoder(const ComputePassEncoder&) = delete;
    ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

public:
    virtual void setPipeline(ComputePipeline& pipeline) = 0;
    virtual void setBindGroup(uint32_t index, BindGroup& bindGroup, std::vector<uint32_t> dynamicOffset = {}) = 0;
    virtual void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) = 0;
    virtual void end() = 0;

protected:
    ComputePassEncoder() = default;
};

} // namespace jipu