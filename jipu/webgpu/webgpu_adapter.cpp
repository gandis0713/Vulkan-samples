#include "webgpu_adapter.h"

#include "webgpu_device.h"
#include "webgpu_instance.h"

namespace jipu
{

WebGPUAdapter* WebGPUAdapter::create(WebGPUInstance* wgpuInstance, WGPU_NULLABLE WGPURequestAdapterOptions const* options)
{
    std::unique_ptr<Instance> instance = nullptr;
    switch (options->backendType)
    {
    case WGPUBackendType::WGPUBackendType_Vulkan: {
        instance = Instance::create({ .type = InstanceType::kVulkan });
    }
    break;
    default:
        // TODO: log error
        return nullptr;
    }

    // TODO: get correct physical device
    std::unique_ptr<PhysicalDevice> physicalDevice = std::move(instance->getPhysicalDevices()[0]);

    if (options)
    {
        return new WebGPUAdapter(wgpuInstance, std::move(instance), std::move(physicalDevice), options);
    }

    return new WebGPUAdapter(wgpuInstance, std::move(instance), std::move(physicalDevice));
}

WebGPUAdapter::WebGPUAdapter(WebGPUInstance* wgpuInstance, std::unique_ptr<Instance> instance, std::unique_ptr<PhysicalDevice> physicalDevice)
    : m_wgpuInstance(wgpuInstance)
    , m_options({})
    , m_instance(std::move(instance))
    , m_physicalDevice(std::move(physicalDevice))
{
}

WebGPUAdapter::WebGPUAdapter(WebGPUInstance* wgpuInstance, std::unique_ptr<Instance> instance, std::unique_ptr<PhysicalDevice> physicalDevice, WGPURequestAdapterOptions const* options)
    : m_wgpuInstance(wgpuInstance)
    , m_options(*options)
    , m_instance(std::move(instance))
    , m_physicalDevice(std::move(physicalDevice))
{
}

void WebGPUAdapter::requestDevice(WGPUDeviceDescriptor const* descriptor, WGPURequestDeviceCallback callback, void* userdata)
{
    auto device = WebGPUDevice::create(this, descriptor);
    if (device)
    {
        std::string message = "Succeed to create device";
        callback(WGPURequestDeviceStatus::WGPURequestDeviceStatus_Success, reinterpret_cast<WGPUDevice>(device), WGPUStringView{ .data = message.data(), .length = message.length() }, userdata);
    }
    else
    {
        std::string message = "Failed to create device";
        callback(WGPURequestDeviceStatus::WGPURequestDeviceStatus_Error, nullptr, WGPUStringView{ .data = message.data(), .length = message.length() }, userdata);
    }
}

std::shared_ptr<Instance> WebGPUAdapter::getInstance() const
{
    return m_instance;
}

PhysicalDevice* WebGPUAdapter::getPhysicalDevice() const
{
    return m_physicalDevice.get();
}

} // namespace jipu