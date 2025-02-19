#pragma once

#include <unordered_map>
#include <vector>

#include "vulkan_api.h"
#include "vulkan_command.h"

namespace jipu
{

class Buffer;
class TextureView;
class BindGroup;

struct BufferUsageInfo
{
    VkPipelineStageFlags stageFlags = 0;
    VkAccessFlags accessFlags = 0;
};

struct TextureUsageInfo
{
    VkPipelineStageFlags stageFlags = 0;
    VkAccessFlags accessFlags = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t baseMipLevel{ 0 };
    uint32_t mipLevelCount{ 1 };
    uint32_t baseArrayLayer{ 0 };
    uint32_t arrayLayerCount{ 1 };
};

struct ResourceInfo
{
    std::unordered_map<Buffer*, BufferUsageInfo> buffers;
    std::unordered_map<TextureView*, TextureUsageInfo> textureViews;
};

struct OperationResourceInfo
{
    ResourceInfo dst{};
    ResourceInfo src{};
};

struct VulkanResourceTrackingResult
{
    std::vector<OperationResourceInfo> operationResourceInfos{};
};

class VulkanCommandEncoder;
class VulkanCommandResourceTracker final
{
public:
    VulkanCommandResourceTracker() = default;
    ~VulkanCommandResourceTracker() = default;

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
    void executeBundle(ExecuteBundleCommand* command);
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
    VulkanResourceTrackingResult finish();

private:
    std::vector<OperationResourceInfo> m_operationResourceInfos;
    OperationResourceInfo m_currentOperationResourceInfo;
};

} // namespace jipu