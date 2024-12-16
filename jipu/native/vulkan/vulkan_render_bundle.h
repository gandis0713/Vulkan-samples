#pragma once

#include "render_bundle.h"

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
};

} // namespace jipu