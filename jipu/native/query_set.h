#pragma once

#include "export.h"

#include <cstdint>

namespace jipu
{

enum class QueryType
{
    kOcclusion,
    kTimestamp,
};

struct QuerySetDescriptor
{
    QueryType type;
    uint32_t count;
};

class JIPU_EXPORT QuerySet
{
public:
    virtual ~QuerySet() = default;

    QuerySet(const QuerySet&) = delete;
    QuerySet& operator=(const QuerySet&) = delete;

    virtual QueryType getType() const = 0;
    virtual uint32_t getCount() const = 0;

protected:
    QuerySet() = default;
};

} // namespace jipu