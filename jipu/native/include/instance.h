#pragma once

#include "export.h"
#include "physical_device.h"
#include "surface.h"

#include <memory>
#include <string>

namespace jipu
{

enum class InstanceType
{
    kNone,
    kVulkan,
    kMetal,
    kD3D12
};

struct InstanceDescriptor
{
    InstanceType type = InstanceType::kNone;
};

class JIPU_EXPORT Instance
{
public:
    static std::unique_ptr<Instance> create(const InstanceDescriptor& descriptor);

public:
    virtual ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

protected:
    Instance();

public:
    virtual std::vector<std::unique_ptr<PhysicalDevice>> getPhysicalDevices() = 0;
    virtual std::unique_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) = 0;
};

} // namespace jipu
