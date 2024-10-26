#include "vulkan_compute_pass_encoder.h"
#include "vulkan_binding_group.h"
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

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.beginComputePass(&command);
    commandEncodingContext.commands.push_back(std::make_unique<BeginComputePassCommand>(std::move(command)));
}

void VulkanComputePassEncoder::setPipeline(ComputePipeline& pipeline)
{
    SetComputePipelineCommand command{
        { .type = CommandType::kSetComputePipeline },
        .pipeline = &pipeline
    };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    // commandEncodingContext.resourceTracker.setComputePipeline(&command);
    commandEncodingContext.commands.push_back(std::make_unique<SetComputePipelineCommand>(std::move(command)));
}

void VulkanComputePassEncoder::setBindingGroup(uint32_t index, BindingGroup& bindingGroup, std::vector<uint32_t> dynamicOffset)
{
    SetBindGroupCommand command{
        { .type = CommandType::kSetComputeBindGroup },
        .index = index,
        .bindingGroup = &bindingGroup,
        .dynamicOffset = dynamicOffset,
    };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.setComputeBindingGroup(&command);
    commandEncodingContext.commands.push_back(std::make_unique<SetBindGroupCommand>(std::move(command)));
}

void VulkanComputePassEncoder::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    DispatchCommand command{
        { .type = CommandType::kDispatch },
        .x = x,
        .y = y,
        .z = z,
    };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    // commandEncodingContext.resourceTracker.dispatch(&command);
    commandEncodingContext.commands.push_back(std::make_unique<DispatchCommand>(std::move(command)));
}

void VulkanComputePassEncoder::end()
{
    EndComputePassCommand command{
        { .type = CommandType::kEndComputePass },
    };

    auto& commandEncodingContext = downcast(m_commandEncoder)->context();

    commandEncodingContext.commandResourceTracker.endComputePass(&command);
    commandEncodingContext.commands.push_back(std::make_unique<EndComputePassCommand>(std::move(command)));
}

} // namespace jipu