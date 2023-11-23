#pragma once

#include "export.h"
#include "vkt/gpu/device.h"
#include "vkt/gpu/surface.h"

#include <memory>

namespace jipu
{

struct PhysicalDeviceDescriptor
{
    uint32_t index{ 0 }; // Index of physical device. If there is a device, index is 0.
};

class Driver;

class VKT_EXPORT PhysicalDevice
{
public:
    PhysicalDevice() = delete;
    PhysicalDevice(Driver* driver, PhysicalDeviceDescriptor descriptor);
    virtual ~PhysicalDevice() = default;

    virtual std::unique_ptr<Device> createDevice(DeviceDescriptor descriptor) = 0;

    Driver* getDriver() const;

protected:
    Driver* m_driver = nullptr;
};

} // namespace jipu
