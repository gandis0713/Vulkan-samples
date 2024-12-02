#pragma once

#include <filesystem>
#include <fmt/format.h>

namespace fmt
{
template <>
struct formatter<std::filesystem::path>
{
    constexpr auto parse(format_parse_context& context)
    {
        return context.begin();
    }

    template <typename T>
    auto format(const std::filesystem::path& path, T& t) const
    {
        return fmt::format_to(t.out(), "{:s}", path.c_str());
    }
};
} // namespace fmt