#pragma once

#include "export.h"
#include "surface.h"
#include "texture.h"
#include "texture_view.h"

#include <memory>
#include <vector>

namespace jipu
{

class Queue;
class Surface;
struct SwapchainDescriptor
{
    Surface* surface = nullptr;
    TextureFormat textureFormat = TextureFormat::kUndefined;
    PresentMode presentMode = PresentMode::kUndefined;
    ColorSpace colorSpace = ColorSpace::kUndefined;
    uint32_t width = 0;
    uint32_t height = 0;

    // for Vulkan and D3D12
    Queue* queue = nullptr;
};

class JIPU_EXPORT Swapchain
{
public:
    virtual ~Swapchain() noexcept = default;

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

public:
    virtual TextureFormat getTextureFormat() const = 0;
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;

    virtual void present() = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;

    virtual Texture* acquireNextTexture() = 0;
    virtual TextureView* acquireNextTextureView() = 0;

protected:
    Swapchain() = default;
};

} // namespace jipu
