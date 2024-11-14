#pragma once

#include "export.h"
#include "physical_device.h"
#include "surface.h"

#include <memory>
#include <string>

namespace jipu
{

enum class AdapterType
{
    kNone,
    kVulkan,
    kMetal,
    kD3D12
};

struct AdapterDescriptor
{
    AdapterType type = AdapterType::kNone;
};

class JIPU_EXPORT Adapter
{
public:
    static std::unique_ptr<Adapter> create(const AdapterDescriptor& descriptor);

public:
    virtual ~Adapter();

    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

protected:
    Adapter();

public:
    virtual std::vector<std::unique_ptr<PhysicalDevice>> getPhysicalDevices() = 0;
    virtual std::unique_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) = 0;
};

} // namespace jipu
