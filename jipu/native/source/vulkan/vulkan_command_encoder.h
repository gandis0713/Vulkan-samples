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
    std::vector<PassResourceInfo> passResourceInfos{};
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

    void copyBufferToBuffer(const BlitBuffer& src,
                            const BlitBuffer& dst,
                            uint64_t size) override;
    void copyBufferToTexture(const BlitTextureBuffer& buffer,
                             const BlitTexture& texture,
                             const Extent3D& extent) override;
    void copyTextureToBuffer(const BlitTexture& texture,
                             const BlitTextureBuffer& buffer,
                             const Extent3D& extent) override;
    void copyTextureToTexture(const BlitTexture& src,
                              const BlitTexture& dst,
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
    VulkanDevice* getDevice() const;
    CommandEncodingContext& context();
    CommandEncodingResult result();

private:
    VulkanDevice* m_device = nullptr;
    CommandEncodingContext m_commandEncodingContext{};
};
DOWN_CAST(VulkanCommandEncoder, CommandEncoder);

} // namespace jipu