#pragma once

#include <unordered_map>
#include <vector>

#include "vulkan_api.h"
#include "vulkan_command.h"
#include "vulkan_resource_tracker.h"

namespace jipu
{

class Buffer;
class Texture;
class BindingGroup;
class VulkanCommandBuffer;

struct VulkanResourceSynchronizerDescriptor
{
    std::vector<PassResourceInfo> passResourceInfos{};
};

class VulkanResourceSynchronizer final
{
public:
    VulkanResourceSynchronizer() = default;
    VulkanResourceSynchronizer(VulkanCommandBuffer* commandBuffer, const VulkanResourceSynchronizerDescriptor& descriptor);
    ~VulkanResourceSynchronizer() = default;

public:
    // compute pass
    void beginComputePass(BeginComputePassCommand* command);
    void setComputePipeline(SetComputePipelineCommand* command);
    void setComputeBindingGroup(SetBindGroupCommand* command);
    void dispatch(DispatchCommand* command);
    void dispatchIndirect(DispatchIndirectCommand* command);
    void endComputePass(EndComputePassCommand* command);

    // render pass
    void beginRenderPass(BeginRenderPassCommand* command);
    void setRenderPipeline(SetRenderPipelineCommand* command);
    void setRenderBindingGroup(SetBindGroupCommand* command);
    void setVertexBuffer(SetVertexBufferCommand* command);
    void setIndexBuffer(SetIndexBufferCommand* command);
    void setViewport(SetViewportCommand* command);
    void setScissor(SetScissorCommand* command);
    void setBlendConstant(SetBlendConstantCommand* command);
    void draw(DrawCommand* command);
    void drawIndexed(DrawIndexedCommand* command);
    void beginOcclusionQuery(BeginOcclusionQueryCommand* command);
    void endOcclusionQuery(EndOcclusionQueryCommand* command);
    void endRenderPass(EndRenderPassCommand* command);

    // copy
    void copyBufferToBuffer(CopyBufferToBufferCommand* command);
    void copyBufferToTexture(CopyBufferToTextureCommand* command);
    void copyTextureToBuffer(CopyTextureToBufferCommand* command);
    void copyTextureToTexture(CopyTextureToTextureCommand* command);

    // query
    void resolveQuerySet(ResolveQuerySetCommand* command);

public:
    bool findProducedBuffer(Buffer* buffer) const;
    bool findProducedTexture(Texture* texture) const;
    BufferUsageInfo extractProducedBufferUsageInfo(Buffer* buffer);
    TextureUsageInfo extractProducedTextureUsageInfo(Texture* texture);

private:
    void increasePassIndex();
    int32_t currentPassIndex() const;

private:
    struct PipelineBarrier
    {
        VkPipelineStageFlags srcStageMask;
        VkPipelineStageFlags dstStageMask;
        VkDependencyFlags dependencyFlags;
        std::vector<VkMemoryBarrier> memoryBarriers{};
        std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers{};
        std::vector<VkImageMemoryBarrier> imageMemoryBarriers{};
    };

    void sync(const PipelineBarrier& barrier);

private:
    VulkanCommandBuffer* m_commandBuffer = nullptr;
    VulkanResourceSynchronizerDescriptor m_descriptor{};
    int32_t m_currentPassIndex = -1;
};

} // namespace jipu