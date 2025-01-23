#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vulkan_api.h"
#include "vulkan_command.h"
#include "vulkan_command_resource_tracker.h"

namespace jipu
{

class Buffer;
class Texture;
class BindGroup;
class VulkanCommandRecorder;

struct VulkanCommandResourceSynchronizerDescriptor
{
    std::vector<OperationResourceInfo> operationResourceInfos{};
};

struct ResourceSyncResult
{
    std::vector<OperationResourceInfo> notSyncedOperationResourceInfos{};
};

class VulkanCommandResourceSynchronizer final
{
public:
    VulkanCommandResourceSynchronizer() = default;
    VulkanCommandResourceSynchronizer(VulkanCommandRecorder* commandRecorder, const VulkanCommandResourceSynchronizerDescriptor& descriptor);
    ~VulkanCommandResourceSynchronizer() = default;

public:
    // compute pass
    void beginComputePass(BeginComputePassCommand* command);
    void setComputePipeline(SetComputePipelineCommand* command);
    void setComputeBindGroup(SetBindGroupCommand* command);
    void dispatch(DispatchCommand* command);
    void dispatchIndirect(DispatchIndirectCommand* command);
    void endComputePass(EndComputePassCommand* command);

    // render pass
    void beginRenderPass(BeginRenderPassCommand* command);
    void setRenderPipeline(SetRenderPipelineCommand* command);
    void setRenderBindGroup(SetBindGroupCommand* command);
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
    ResourceSyncResult finish();

private:
    bool findSrcBuffer(Buffer* buffer) const;
    bool findSrcTextureView(TextureView* textureView) const;
    BufferUsageInfo extractSrcBufferUsageInfo(Buffer* buffer);
    TextureUsageInfo extractSrcTextureUsageInfo(TextureView* textureView);

    void increaseOperationIndex();
    int32_t currentOperationIndex() const;
    OperationResourceInfo& getCurrentOperationResourceInfo();

private:
    void sync();

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

    void cmdPipelineBarrier(const PipelineBarrier& barrier);

private:
    VulkanCommandRecorder* m_commandRecorder = nullptr;
    std::vector<OperationResourceInfo> m_operationResourceInfos{};
    int32_t m_currentOperationIndex = -1;

    struct
    {
        std::unordered_set<Buffer*> buffers{};
        std::unordered_set<TextureView*> textureViews{};
        void clear()
        {
            buffers.clear();
            textureViews.clear();
        }
    } m_activatedDstResource;
};

} // namespace jipu