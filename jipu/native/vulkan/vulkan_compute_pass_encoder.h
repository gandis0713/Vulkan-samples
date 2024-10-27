#pragma once

#include "compute_pass_encoder.h"
#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_pipeline.h"

namespace jipu
{

class VulkanBindGroup;
class VulkanComputePipeline;
class VulkanCommandEncoder;
class VULKAN_EXPORT VulkanComputePassEncoder : public ComputePassEncoder
{
public:
    VulkanComputePassEncoder() = delete;
    VulkanComputePassEncoder(VulkanCommandEncoder* commandEncoder, const ComputePassEncoderDescriptor& descriptor);
    ~VulkanComputePassEncoder() override = default;

public:
    void setPipeline(ComputePipeline& pipeline) override;
    void setBindGroup(uint32_t index, BindGroup& bindGroup, std::vector<uint32_t> dynamicOffset = {}) override;
    void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) override;
    void end() override;

private:
    VulkanCommandEncoder* m_commandEncoder = nullptr;

private:
    std::optional<VulkanComputePipeline::Ref> m_pipeline = std::nullopt;
};

} // namespace jipu