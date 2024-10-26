#pragma once

#include "compute_pass_encoder.h"
#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_pipeline.h"

namespace jipu
{

class VulkanBindingGroup;
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
    void setBindingGroup(uint32_t index, BindingGroup& bindingGroup, std::vector<uint32_t> dynamicOffset = {}) override;
    void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) override;
    void end() override;

private:
    VulkanCommandEncoder* m_commandEncoder = nullptr;

private:
    std::optional<VulkanComputePipeline::Ref> m_pipeline = std::nullopt;
};

} // namespace jipu