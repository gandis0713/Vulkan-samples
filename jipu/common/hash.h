#pragma once

#include <functional>

namespace jipu
{

template <typename T>
inline size_t hash(const T& value)
{
    return std::hash<T>()(value);
}

template <class T>
inline void combineHash(size_t& seed, const T& value) noexcept
{
    seed ^= std::hash<T>()(value) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
}

} // namespace jipu