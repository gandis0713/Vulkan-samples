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

protected:
    PhysicalDevice() = default;

public:
    virtual std::unique_ptr<Device> createDevice(const DeviceDescriptor& descriptor) = 0;

public:
    virtual Adapter* getAdapter() const = 0;
    virtual PhysicalDeviceInfo getPhysicalDeviceInfo() const = 0;
    virtual SurfaceCapabilities getSurfaceCapabilities(Surface* surface) const = 0;
};

} // namespace jipu
