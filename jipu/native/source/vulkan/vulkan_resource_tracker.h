#pragma once

#include <unordered_map>
#include <vector>

#include "vulkan_api.h"
#include "vulkan_command.h"

namespace jipu
{

class Buffer;
class Texture;
class BindingGroup;

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
};

struct PassResourceInfo
{
    struct Source // consumer
    {
        std::unordered_map<Buffer*, BufferUsageInfo> buffers;
        std::unordered_map<Texture*, TextureUsageInfo> textures;
    } dst;

    struct Destination // producer
    {
        std::unordered_map<Buffer*, BufferUsageInfo> buffers;
        std::unordered_map<Texture*, TextureUsageInfo> textures;
    } src;

    void clear()
    {
        dst.buffers.clear();
        dst.textures.clear();

        src.buffers.clear();
        src.textures.clear();
    }
};

class VulkanCommandEncoder;
class VulkanResourceTracker final
{
public:
    VulkanResourceTracker() = default;
    ~VulkanResourceTracker() = default;

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
    std::vector<PassResourceInfo> extractPassResourceInfos();

private:
    std::vector<PassResourceInfo> m_passResourceInfos;
    PassResourceInfo m_ongoingPassResourceInfo;
};

} // namespace jipu