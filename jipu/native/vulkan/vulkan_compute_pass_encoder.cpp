#include "vulkan_compute_pass_encoder.h"
#include "vulkan_bind_group.h"
#include "vulkan_buffer.h"
#include "vulkan_command_encoder.h"
#include "vulkan_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_pipeline_layout.h"

#include "vulkan_api.h"

namespace jipu
{

VulkanComputePassEncoder::VulkanComputePassEncoder(VulkanCommandEncoder* commandEncoder, const ComputePassEncoderDescriptor& descriptor)
    : m_commandEncoder(commandEncoder)
{
    BeginComputePassCommand command{
        { .type = CommandType::kBeginComputePass }
    };

    m_commandEncoder->addCommand(std::make_unique<BeginComputePassCommand>(std::move(command)));
}

void VulkanComputePassEncoder::setPipeline(ComputePipeline* pipeline)
{
    SetComputePipelineCommand command{
        { .type = CommandType::kSetComputePipeline },
        .pipeline = pipeline
    };

    m_commandEncoder->addCommand(std::make_unique<SetComputePipelineCommand>(std::move(command)));
}

void VulkanComputePassEncoder::setBindGroup(uint32_t index, BindGroup* bindGroup, std::vector<uint32_t> dynamicOffset)
{
    SetBindGroupCommand command{
        { .type = CommandType::kSetComputeBindGroup },
        .index = index,
        .bindGroup = bindGroup,
        .dynamicOffset = dynamicOffset,
    };

    m_commandEncoder->addCommand(std::make_unique<SetBindGroupCommand>(std::move(command)));
}

void VulkanComputePassEncoder::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    DispatchCommand command{
        { .type = CommandType::kDispatch },
        .x = x,
        .y = y,
        .z = z,
    };

    m_commandEncoder->addCommand(std::make_unique<DispatchCommand>(std::move(command)));
}

void VulkanComputePassEncoder::end()
{
    EndComputePassCommand command{
        { .type = CommandType::kEndComputePass },
    };

    m_commandEncoder->addCommand(std::make_unique<EndComputePassCommand>(std::move(command)));
}

} // namespace jipu