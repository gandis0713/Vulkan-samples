#pragma once

#include "handle.h"
#include "hpc/backend/instance.h"
#include "common/cast.h"

namespace hpc
{
namespace backend
{
namespace adreno
{

class AdrenoInstance : public Instance
{
public:
    explicit AdrenoInstance() = default;
    ~AdrenoInstance() override = default;

    std::vector<std::unique_ptr<hpc::backend::GPU>> gpus() override;
};
DOWN_CAST(AdrenoInstance, Instance)

} // namespace adreno
} // namespace backend
} // namespace hpc