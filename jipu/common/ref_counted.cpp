#include "ref_counted.h"

namespace jipu
{

RefCounted::RefCounted()
    : m_count(1)
{
}

void RefCounted::addRef()
{
    // m_count.fetch_add(1, std::memory_order_relaxed);
    ++m_count;
}

void RefCounted::release()
{
    // if (m_count.fetch_sub(1, std::memory_order_acq_rel) <= 1)
    if (--m_count == 0)
    {
        delete this;
    }
}

} // namespace jipu