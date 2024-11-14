#pragma once

#include "export.h"
#include <functional>
#include <stdint.h>
#include <vector>

#include "bind_group_layout.h"

namespace jipu
{

using BindGroupLayouts = std::vector<BindGroupLayout*>;

class Device;
class BindGroupLayout;
struct PipelineLayoutDescriptor
{
    BindGroupLayouts layouts = {};
};

class JIPU_EXPORT PipelineLayout
{
public:
    virtual ~PipelineLayout() = default;

    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;

protected:
    PipelineLayout() = default;
};

} // namespace jipu