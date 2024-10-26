#pragma once

#include "export.h"
#include <functional>
#include <stdint.h>
#include <vector>

#include "binding_group_layout.h"

namespace jipu
{

using BindingGroupLayouts = std::vector<BindingGroupLayout*>;

class Device;
class BindingGroupLayout;
struct PipelineLayoutDescriptor
{
    BindingGroupLayouts layouts = {};
};

class JIPU_EXPORT PipelineLayout
{
public:
    virtual ~PipelineLayout() = default;

protected:
    PipelineLayout() = default;
};

} // namespace jipu