#pragma once

#include "export.h"

#include "command_encoder.h"
#include "compute_pass_encoder.h"
#include "jipu/common/cast.h"
#include "render_pass_encoder.h"

#include "vulkan_api.h"
#include "vulkan_command.h"
#include "vulkan_command_resource_tracker.h"
#include "vulkan_export.h"
#include "vulkan_render_pass_encoder.h"

#include <queue>

namespace jipu
{

struct CommandEncodingContext
{
    std::vector<std::unique_ptr<Command>> commands{};
    VulkanCommandResourceTracker commandResourceTracker{};
};

struct CommandEncodingResult
{
    std::vector<std::unique_ptr<Command>> commands{};
    VulkanResourceTrackingResult resourceTrackingResult{};
};

class VulkanDevice;
class VULKAN_EXPORT VulkanCommandEncoder : public CommandEncoder
{
public:
    VulkanCommandEncoder() = delete;
    VulkanCommandEncoder(VulkanDevice* device, const CommandEncoderDescriptor& descriptor);
    ~VulkanCommandEncoder() override = default;

    std::unique_ptr<ComputePassEncoder> beginComputePass(const ComputePassEncoderDescriptor& descriptor) override;
    std::unique_ptr<RenderPassEncoder> beginRenderPass(const RenderPassEncoderDescriptor& descriptor) override;

    void copyBufferToBuffer(const CopyBuffer& src,
                            const CopyBuffer& dst,
                            uint64_t size) override;
    void copyBufferToTexture(const CopyTextureBuffer& buffer,
                             const CopyTexture& texture,
                             const Extent3D& extent) override;
    void copyTextureToBuffer(const CopyTexture& texture,
                             const CopyTextureBuffer& buffer,
                             const Extent3D& extent) override;
    void copyTextureToTexture(const CopyTexture& src,
                              const CopyTexture& dst,
                              const Extent3D& extent) override;
    void resolveQuerySet(QuerySet* querySet,
                         uint32_t firstQuery,
                         uint32_t queryCount,
                         Buffer* destination,
                         uint64_t destinationOffset) override;

    std::unique_ptr<CommandBuffer> finish(const CommandBufferDescriptor& descriptor) override;

public:
    std::unique_ptr<RenderPassEncoder> beginRenderPass(const VulkanRenderPassEncoderDescriptor& descriptor);

public:
    void addCommand(std::unique_ptr<Command> command);
    CommandEncodingResult finish();

public:
    VulkanDevice* getDevice() const;

private:
    VulkanDevice* m_device = nullptr;

private:
    std::vector<std::unique_ptr<Command>> m_commands{};
    VulkanCommandResourceTracker m_commandResourceTracker{};
};
DOWN_CAST(VulkanCommandEncoder, CommandEncoder);

} // namespace jipu