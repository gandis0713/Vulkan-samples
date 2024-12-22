#pragma once

#include "render_bundle.h"
#include "vulkan_api.h"

#include <memory>

namespace jipu
{

class VulkanRenderBundle : public RenderBundle
{
public:
    static std::unique_ptr<RenderBundle> create(const RenderBundleDescriptor& descriptor);

public:
    VulkanRenderBundle() = delete;
    ~VulkanRenderBundle() override = default;

    VulkanRenderBundle(const VulkanRenderBundle&) = delete;
    VulkanRenderBundle& operator=(const VulkanRenderBundle&) = delete;

private:
    VulkanRenderBundle(const RenderBundleDescriptor& descriptor);

private:
    // Secondary command buffer.
    [[maybe_unused]] VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
};

} // namespace jipu