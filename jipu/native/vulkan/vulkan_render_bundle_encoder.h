#pragma once

#include "jipu/common/cast.h"
#include "render_bundle_encoder.h"
#include "vulkan_command.h"
#include "vulkan_command_encoder.h"
#include "vulkan_command_resource_tracker.h"

namespace jipu
{

struct RenderBundleEncodingResult
{
    std::vector<std::unique_ptr<Command>> commands{};
};

class VulkanDevice;
class VulkanRenderBundleEncoder : public RenderBundleEncoder
{
public:
    static std::unique_ptr<RenderBundleEncoder> create(VulkanDevice* device, const RenderBundleEncoderDescriptor& descriptor);

public:
    VulkanRenderBundleEncoder() = delete;
    ~VulkanRenderBundleEncoder() override = default;

    VulkanRenderBundleEncoder(const VulkanRenderBundleEncoder&) = delete;
    VulkanRenderBundleEncoder& operator=(const VulkanRenderBundleEncoder&) = delete;

public:
    void setPipeline(RenderPipeline* pipeline) override;
    void setBindGroup(uint32_t index, BindGroup* bindGroup, std::vector<uint32_t> dynamicOffset = {}) override;
    void setVertexBuffer(uint32_t slot, Buffer* buffer) override;
    void setIndexBuffer(Buffer* buffer, IndexFormat format) override;

    void draw(uint32_t vertexCount,
              uint32_t instanceCount,
              uint32_t firstVertex,
              uint32_t firstInstance) override;

    void drawIndexed(uint32_t indexCount,
                     uint32_t instanceCount,
                     uint32_t indexOffset,
                     uint32_t vertexOffset,
                     uint32_t firstInstance) override;

    std::unique_ptr<RenderBundle> finish(const RenderBundleDescriptor& descriptor) override;

public:
    VulkanDevice* getDevice() const;

private:
    VulkanRenderBundleEncoder(VulkanDevice* device, const RenderBundleEncoderDescriptor& descriptor);

private:
    void addCommand(std::unique_ptr<Command> command);
    CommandEncodingResult extractResult();

private:
    [[maybe_unused]] VulkanDevice* m_device = nullptr;
    [[maybe_unused]] const RenderBundleEncoderDescriptor m_descriptor;

private:
    std::vector<std::unique_ptr<Command>> m_commands{};

    friend class VulkanRenderBundle;
};

DOWN_CAST(VulkanRenderBundleEncoder, RenderBundleEncoder);

} // namespace jipu