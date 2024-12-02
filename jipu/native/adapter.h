#pragma once

#include "export.h"
#include "physical_device.h"
#include "surface.h"

#include <memory>
#include <string>

namespace jipu
{

enum class BackendAPI
{
    kNone,
    kVulkan,
    kMetal,
    kD3D12
};

struct AdapterDescriptor
{
    BackendAPI type = BackendAPI::kNone;
};

class Instance;
class JIPU_EXPORT Adapter
{
public:
    virtual ~Adapter() = default;

    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

public:
    virtual std::vector<std::unique_ptr<PhysicalDevice>> getPhysicalDevices() = 0;
    virtual std::unique_ptr<Surface> createSurface(const SurfaceDescriptor& descriptor) = 0;

public:
    virtual Instance* getInstance() const = 0;

protected:
    Adapter() = default;
};

} // namespace jipu
