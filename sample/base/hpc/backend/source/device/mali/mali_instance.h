#pragma once

#include "common/cast.h"
#include "handle.h"
#include "hpc/backend/instance.h"

namespace hpc
{
namespace backend
{
namespace mali
{

class MaliInstance : public Instance
{
public:
    explicit MaliInstance() = default;
    ~MaliInstance() override = default;

    std::vector<std::unique_ptr<hpc::backend::GPU>> gpus() override;
};
DOWN_CAST(MaliInstance, Instance)

} // namespace mali
} // namespace backend
} // namespace hpc