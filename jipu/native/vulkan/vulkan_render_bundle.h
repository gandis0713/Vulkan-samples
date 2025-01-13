#pragma once

#include "jipu/common/cast.h"
#include "render_bundle.h"
#include "vulkan_api.h"
#include "vulkan_command_recorder.h"

#include <memory>

namespace jipu
{

class VulkanDevice;
class VulkanRenderBundleEncoder;
class VulkanRenderBundle : public RenderBundle
{
public:
    static std::unique_ptr<RenderBundle> create(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor);

public:
    VulkanRenderBundle() = delete;
    ~VulkanRenderBundle() override = default;

    VulkanRenderBundle(const VulkanRenderBundle&) = delete;
    VulkanRenderBundle& operator=(const VulkanRenderBundle&) = delete;

public:
    const std::vector<std::unique_ptr<Command>>& getCommands() const;

private:
    VulkanRenderBundle(VulkanRenderBundleEncoder* renderBundleEncoder, const RenderBundleDescriptor& descriptor);

private:
    [[maybe_unused]] VulkanDevice* m_device = nullptr;
    [[maybe_unused]] const RenderBundleDescriptor m_descriptor;

    CommandEncodingResult m_commandEncodingResult{};

private:
    [[maybe_unused]] VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE; // Secondary command buffer.
};

DOWN_CAST(VulkanRenderBundle, RenderBundle);

} // namespace jipu