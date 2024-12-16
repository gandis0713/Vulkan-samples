#include "webgpu_adapter.h"

#include "webgpu_adapter.h"
#include "webgpu_device.h"
#include "webgpu_instance.h"

#include "event/request_device_event.h"

namespace jipu
{

WebGPUAdapter* WebGPUAdapter::create(WebGPUInstance* wgpuInstance, WGPU_NULLABLE WGPURequestAdapterOptions const* options)
{
    WGPURequestAdapterOptions wgpuOptions = options ? *options : WGPURequestAdapterOptions{};

    std::unique_ptr<Adapter> adapter = wgpuInstance->getInstance()->createAdapter({ .type = ToBackendAPI(wgpuOptions.backendType) });
    if (!adapter)
    {
        // TODO: log or throw error
        return nullptr;
    }

    // TODO: get correct physical device by adapter options
    std::unique_ptr<PhysicalDevice> physicalDevice = std::move(adapter->getPhysicalDevices()[0]);

    return new WebGPUAdapter(wgpuInstance, std::move(adapter), std::move(physicalDevice), wgpuOptions);
}

WebGPUAdapter::WebGPUAdapter(WebGPUInstance* wgpuInstance, std::unique_ptr<Adapter> adapter, std::unique_ptr<PhysicalDevice> physicalDevice, const WGPURequestAdapterOptions& options)
    : m_wgpuInstance(wgpuInstance)
    , m_options(options)
    , m_adapter(std::move(adapter))
    , m_physicalDevice(std::move(physicalDevice))
{
}

WGPUFuture WebGPUAdapter::requestDevice(WGPUDeviceDescriptor const* descriptor, WGPURequestDeviceCallbackInfo2 callbackInfo)
{
    auto device = WebGPUDevice::create(this, descriptor);

    return WGPUFuture{ .id = m_wgpuInstance->getEventManager()->addEvent(RequestDeviceEvent::create(device, callbackInfo)) };
}

WebGPUInstance* WebGPUAdapter::getInstance() const
{
    return m_wgpuInstance;
}

std::shared_ptr<Adapter> WebGPUAdapter::getAdapter() const
{
    return m_adapter;
}

PhysicalDevice* WebGPUAdapter::getPhysicalDevice() const
{
    return m_physicalDevice.get();
}

// Convert from JIPU to WebGPU
WGPUBackendType ToWGPUBackendAPI(BackendAPI api)
{
    switch (api)
    {
    case BackendAPI::kMetal:
        return WGPUBackendType_Metal;
    case BackendAPI::kD3D12:
        return WGPUBackendType_D3D12;
    case BackendAPI::kVulkan:
        return WGPUBackendType_Vulkan;
    default:
    case BackendAPI::kNone:
        return WGPUBackendType_Undefined;
    }
}

// Convert from WebGPU to JIPU
BackendAPI ToBackendAPI(WGPUBackendType api)
{
    switch (api)
    {
    case WGPUBackendType_Metal:
        return BackendAPI::kMetal;
    case WGPUBackendType_D3D12:
        return BackendAPI::kD3D12;
    case WGPUBackendType_Vulkan:
        return BackendAPI::kVulkan;
    case WGPUBackendType_Undefined:
    case WGPUBackendType_OpenGL:
    case WGPUBackendType_OpenGLES:
    case WGPUBackendType_D3D11:
    case WGPUBackendType_WebGPU:
    case WGPUBackendType_Null:
    default:
        return BackendAPI::kNone;
    }
}

} // namespace jipu