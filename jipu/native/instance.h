#pragma once

#include "export.h"
#include <memory>

#include "jipu/native/adapter.h"

namespace jipu
{

struct InstanceDescriptor
{
};

class JIPU_EXPORT Instance final
{

public:
    static std::unique_ptr<Instance> create(const InstanceDescriptor& descriptor);

public:
    Instance() = delete;
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

public:
    std::unique_ptr<Adapter> createAdapter(const AdapterDescriptor& descriptor);

private:
    Instance(const InstanceDescriptor& descriptor) noexcept;

private:
    [[maybe_unused]] const InstanceDescriptor m_descriptor{};
};

} // namespace jipu