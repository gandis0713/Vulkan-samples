#pragma once

#include "device.h"
#include "export.h"
#include "surface.h"

#include <memory>

namespace jipu
{

struct PhysicalDeviceInfo
{
    std::string deviceName;
};

class Adapter;
class JIPU_EXPORT PhysicalDevice
{
public:
    virtual ~PhysicalDevice() = default;

    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;

public:
    virtual std::unique_ptr<Device> createDevice(const DeviceDescriptor& descriptor) = 0;

public:
    virtual PhysicalDeviceInfo getPhysicalDeviceInfo() const = 0;
    virtual SurfaceCapabilities getSurfaceCapabilities(Surface* surface) const = 0;

public:
    virtual Adapter* getAdapter() const = 0;

protected:
    PhysicalDevice() = default;
};

} // namespace jipu
