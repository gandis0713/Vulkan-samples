#include "webgpu_adapter.h"

#include "webgpu_adapter.h"
#include "webgpu_device.h"

namespace jipu
{

WebGPUAdapter* WebGPUAdapter::create(WebGPUInstance* wgpuInstance, WGPU_NULLABLE WGPURequestAdapterOptions const* options)
{
    std::unique_ptr<Adapter> adapter = nullptr;
    switch (options->backendType)
    {
    case WGPUBackendType::WGPUBackendType_Vulkan: {
        adapter = Adapter::create({ .type = AdapterType::kVulkan });
    }
    break;
    default:
        // TODO: log error
        return nullptr;
    }

    // TODO: get correct physical device
    std::unique_ptr<PhysicalDevice> physicalDevice = std::move(adapter->getPhysicalDevices()[0]);

    if (options)
    {
        return new WebGPUAdapter(wgpuInstance, std::move(adapter), std::move(physicalDevice), options);
    }

    return new WebGPUAdapter(wgpuInstance, std::move(adapter), std::move(physicalDevice));
}

WebGPUAdapter::WebGPUAdapter(WebGPUInstance* wgpuInstance, std::unique_ptr<Adapter> adapter, std::unique_ptr<PhysicalDevice> physicalDevice)
    : m_wgpuInstance(wgpuInstance)
    , m_options({})
    , m_adapter(std::move(adapter))
    , m_physicalDevice(std::move(physicalDevice))
{
}

WebGPUAdapter::WebGPUAdapter(WebGPUInstance* wgpuInstance, std::unique_ptr<Adapter> adapter, std::unique_ptr<PhysicalDevice> physicalDevice, WGPURequestAdapterOptions const* options)
    : m_wgpuInstance(wgpuInstance)
    , m_options(*options)
    , m_adapter(std::move(adapter))
    , m_physicalDevice(std::move(physicalDevice))
{
}

void WebGPUAdapter::requestDevice(WGPUDeviceDescriptor const* descriptor, WGPURequestDeviceCallback callback, void* userdata)
{
    auto device = WebGPUDevice::create(this, descriptor);
    if (device)
    {
        std::string message = "Succeed to create device";
        callback(WGPURequestDeviceStatus::WGPURequestDeviceStatus_Success, reinterpret_cast<WGPUDevice>(device), WGPUStringView{ .data = message.data(), .length = message.size() }, userdata);
    }
    else
    {
        std::string message = "Failed to create device";
        callback(WGPURequestDeviceStatus::WGPURequestDeviceStatus_Error, nullptr, WGPUStringView{ .data = message.data(), .length = message.size() }, userdata);
    }
}

std::shared_ptr<Adapter> WebGPUAdapter::getAdapter() const
{
    return m_adapter;
}

PhysicalDevice* WebGPUAdapter::getPhysicalDevice() const
{
    return m_physicalDevice.get();
}

} // namespace jipu