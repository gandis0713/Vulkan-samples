#pragma once

#include "webgpu_header.h"

#include "jipu/common/ref_counted.h"

#include "jipu/native/include/instance.h"
#include "jipu/native/include/physical_device.h"

namespace jipu
{

class WebGPUInstance;
class WebGPUAdapter : public RefCounted
{
public:
    static WebGPUAdapter* create(WebGPUInstance* wgpuInstance, WGPU_NULLABLE WGPURequestAdapterOptions const* options);

public:
    WebGPUAdapter() = delete;
    explicit WebGPUAdapter(WebGPUInstance* wgpuInstance,
                           std::unique_ptr<Instance> instance,
                           std::unique_ptr<PhysicalDevice> physicalDevice);
    explicit WebGPUAdapter(WebGPUInstance* wgpuInstance,
                           std::unique_ptr<Instance> instance,
                           std::unique_ptr<PhysicalDevice> physicalDevice,
                           WGPURequestAdapterOptions const* options);

public:
    virtual ~WebGPUAdapter() = default;

    WebGPUAdapter(const WebGPUAdapter&) = delete;
    WebGPUAdapter& operator=(const WebGPUAdapter&) = delete;

public: // WebGPU API
    void requestDevice(WGPUDeviceDescriptor const* descriptor, WGPURequestDeviceCallback callback, void* userdata);

public:
    std::shared_ptr<Instance> getInstance() const;
    PhysicalDevice* getPhysicalDevice() const;

private:
    [[maybe_unused]] WebGPUInstance* m_wgpuInstance = nullptr;
    [[maybe_unused]] const WGPURequestAdapterOptions m_options{};

private:
    std::shared_ptr<Instance> m_instance = nullptr; // shared with surface.
    std::unique_ptr<PhysicalDevice> m_physicalDevice = nullptr;
};

} // namespace jipu