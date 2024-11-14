#include "webgpu_surface.h"

#include "webgpu_adapter.h"
#include "webgpu_device.h"
#include "webgpu_instance.h"
#include "webgpu_queue.h"
#include "webgpu_texture.h"

namespace jipu
{

WebGPUSurface* WebGPUSurface::create(WebGPUInstance* instance, WGPUSurfaceDescriptor const* descriptor)
{
    return new WebGPUSurface(instance, descriptor);
}

WebGPUSurface::WebGPUSurface(WebGPUInstance* instance, WGPUSurfaceDescriptor const* descriptor)
    : m_wgpuInstance(instance)
    , m_descriptor(*descriptor)
    , m_configuration({})
    , m_surface(nullptr)
    , m_swapchain(nullptr)
{
    const WGPUChainedStruct* current = m_descriptor.nextInChain;
    while (current)
    {
        switch (current->sType)
        {
        case WGPUSType_SurfaceSourceMetalLayer: {
            m_type = Type::kMetalLayer;
            WGPUSurfaceDescriptorFromMetalLayer const* metalLayer = reinterpret_cast<WGPUSurfaceDescriptorFromMetalLayer const*>(current);
            m_meterLayer = reinterpret_cast<void*>(metalLayer->layer);
        }
        break;
        case WGPUSType_SurfaceSourceAndroidNativeWindow: {
            m_type = Type::kAndroidWindow;
            WGPUSurfaceDescriptorFromAndroidNativeWindow const* androidWindow = reinterpret_cast<WGPUSurfaceDescriptorFromAndroidNativeWindow const*>(current);
            m_androidNativeWindow = reinterpret_cast<void*>(androidWindow->window);
        }
        break;
        case WGPUSType_SurfaceSourceWindowsHWND: {
            m_type = Type::kWindowsHWND;
            WGPUSurfaceDescriptorFromWindowsHWND const* windowHWND = reinterpret_cast<WGPUSurfaceDescriptorFromWindowsHWND const*>(current);
            m_hInstance = reinterpret_cast<void*>(windowHWND->hinstance);
            m_HWND = reinterpret_cast<void*>(windowHWND->hwnd);
        }
        break;
        default:
            // TODO: wayland, Xlib and so on.
            break;
        }

        current = current->next;
    }
}

WGPUStatus WebGPUSurface::getCapabilities(WebGPUAdapter* adapter, WGPUSurfaceCapabilities* capabilities)
{
    // create surface
    if (m_surface == nullptr)
    {
        m_adapter = adapter->getAdapter();

        switch (m_type)
        {
        case Type::kMetalLayer:
            m_surface = m_adapter->createSurface(SurfaceDescriptor{ .windowHandle = m_meterLayer });
            break;
        case Type::kWindowsHWND:
            m_surface = m_adapter->createSurface(SurfaceDescriptor{ .windowHandle = m_HWND });
            break;
        case Type::kAndroidWindow:
            m_surface = m_adapter->createSurface(SurfaceDescriptor{ .windowHandle = m_androidNativeWindow });
            break;
        default:
            break;
        }
    }

    // gather surface capabilities
    auto physicalDevice = adapter->getPhysicalDevice();
    auto surfaceCapabilities = physicalDevice->getSurfaceCapabilities(m_surface.get());
    {
        // formats
        {
            auto formatSize = surfaceCapabilities.formats.size();
            auto formatArray = new WGPUTextureFormat[formatSize];
            for (auto i = 0; i < formatSize; ++i)
            {
                formatArray[i] = ToWGPUTextureFormat(surfaceCapabilities.formats[i]);
            }
            capabilities->formats = formatArray;
            capabilities->formatCount = formatSize;
        }

        // presentModes
        {
            auto presentModeSize = surfaceCapabilities.presentModes.size();
            auto presentModeArray = new WGPUPresentMode[presentModeSize];
            for (auto i = 0; i < presentModeSize; ++i)
            {
                presentModeArray[i] = static_cast<WGPUPresentMode>(surfaceCapabilities.presentModes[i]);
            }
            capabilities->presentModes = presentModeArray;
            capabilities->presentModeCount = presentModeSize;
        }

        // alphaModes
        {
            auto compositeAlphaSize = surfaceCapabilities.compositeAlphaFlags.size();
            auto compositeAlphaArray = new WGPUCompositeAlphaMode[compositeAlphaSize];
            for (auto i = 0; i < compositeAlphaSize; ++i)
            {
                compositeAlphaArray[i] = ToWGPUCompositeAlphaMode(surfaceCapabilities.compositeAlphaFlags[i]);
            }
            capabilities->alphaModes = compositeAlphaArray;
            capabilities->alphaModeCount = compositeAlphaSize;
        }
    }

    return WGPUStatus::WGPUStatus_Success;
}

void WebGPUSurface::configure(WGPUSurfaceConfiguration const* config)
{
    if (m_swapchain == nullptr)
    {
        WebGPUDevice* webgpuDevice = reinterpret_cast<WebGPUDevice*>(config->device);
        WebGPUQueue* webgpuQueue = webgpuDevice->getQueue();

        SwapchainDescriptor descriptor{};
        descriptor.width = config->width;
        descriptor.height = config->height;
        descriptor.textureFormat = WGPUToTextureFormat(config->format);
        descriptor.presentMode = WGPUToPresentMode(config->presentMode);
        descriptor.colorSpace = ColorSpace::kSRGBNonLinear;
        descriptor.surface = m_surface.get();
        descriptor.queue = webgpuQueue->getQueue();

        m_swapchain = webgpuDevice->getOrCreateSwapchain(descriptor);
    }

    m_configuration = *config;
}

void WebGPUSurface::getCurrentTexture(WGPUSurfaceTexture* surfaceTexture)
{
    if (m_swapchain == nullptr)
    {
        surfaceTexture->status = WGPUSurfaceGetCurrentTextureStatus_Error;
        return;
    }

    auto wgpuDevice = reinterpret_cast<WebGPUDevice*>(m_configuration.device);
    auto wgpuQueue = wgpuDevice->getQueue();

    auto currentTexture = m_swapchain->acquireNextTexture();

    surfaceTexture->texture = reinterpret_cast<WGPUTexture>(WebGPUTexture::create(wgpuDevice, currentTexture));
    surfaceTexture->status = WGPUSurfaceGetCurrentTextureStatus_Success;
}

void WebGPUSurface::present()
{
    if (m_swapchain == nullptr)
    {
        return;
    }

    m_swapchain->present();
}

// Convert from JIPU to WebGPU
WGPUCompositeAlphaMode ToWGPUCompositeAlphaMode(CompositeAlphaFlag flag)
{
    switch (flag)
    {
    case CompositeAlphaFlag::kUndefined:
    default:
        return WGPUCompositeAlphaMode_Auto;
    case CompositeAlphaFlag::kOpaque:
        return WGPUCompositeAlphaMode::WGPUCompositeAlphaMode_Opaque;
    case CompositeAlphaFlag::kPreMultiplied:
        return WGPUCompositeAlphaMode::WGPUCompositeAlphaMode_Premultiplied;
    case CompositeAlphaFlag::kPostMultiplied:
        return WGPUCompositeAlphaMode::WGPUCompositeAlphaMode_Unpremultiplied;
    case CompositeAlphaFlag::kInherit:
        return WGPUCompositeAlphaMode::WGPUCompositeAlphaMode_Inherit;
    }
}

WGPUPresentMode ToWGPUPresentMode(PresentMode mode)
{
    switch (mode)
    {
    case PresentMode::kImmediate:
        return WGPUPresentMode::WGPUPresentMode_Immediate;
    case PresentMode::kMailbox:
        return WGPUPresentMode::WGPUPresentMode_Mailbox;
    case PresentMode::kFifo:
        return WGPUPresentMode::WGPUPresentMode_Fifo;
    case PresentMode::kFifoRelaxed:
        return WGPUPresentMode::WGPUPresentMode_FifoRelaxed;
    default:
        return WGPUPresentMode::WGPUPresentMode_Fifo;
    }
}

// Convert from WebGPU to JIPU
CompositeAlphaFlag WPGUToCompositeAlphaMode(WGPUCompositeAlphaMode flag)
{
    switch (flag)
    {
    case WGPUCompositeAlphaMode_Auto:
        return CompositeAlphaFlag::kUndefined;
    case WGPUCompositeAlphaMode_Opaque:
        return CompositeAlphaFlag::kOpaque;
    case WGPUCompositeAlphaMode_Premultiplied:
        return CompositeAlphaFlag::kPreMultiplied;
    case WGPUCompositeAlphaMode_Unpremultiplied:
        return CompositeAlphaFlag::kPostMultiplied;
    case WGPUCompositeAlphaMode_Inherit:
        return CompositeAlphaFlag::kInherit;
    default:
        return CompositeAlphaFlag::kUndefined;
    }
}

PresentMode WGPUToPresentMode(WGPUPresentMode mode)
{
    switch (mode)
    {
    case WGPUPresentMode_Immediate:
        return PresentMode::kImmediate;
    case WGPUPresentMode_Mailbox:
        return PresentMode::kMailbox;
    case WGPUPresentMode_Fifo:
        return PresentMode::kFifo;
    case WGPUPresentMode_FifoRelaxed:
        return PresentMode::kFifoRelaxed;
    default:
        return PresentMode::kFifo;
    }
}

} // namespace jipu