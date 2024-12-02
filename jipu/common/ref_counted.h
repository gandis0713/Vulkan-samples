#pragma once

#include <atomic>

namespace jipu
{

class RefCounted
{

public:
    RefCounted();
    virtual ~RefCounted() = default;

public:
    void addRef();
    void release();

private:
    // std::atomic<uint64_t> m_count = 0;
    uint64_t m_count = 0;
};

} // namespace jipu