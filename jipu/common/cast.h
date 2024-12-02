#pragma once

#define DOWN_CAST(DOWN_CLASS, UP_CLASS)                              \
    inline DOWN_CLASS& downcast(UP_CLASS& that) noexcept             \
    {                                                                \
        return static_cast<DOWN_CLASS&>(that);                       \
    }                                                                \
    inline const DOWN_CLASS& downcast(const UP_CLASS& that) noexcept \
    {                                                                \
        return static_cast<const DOWN_CLASS&>(that);                 \
    }                                                                \
    inline DOWN_CLASS* downcast(UP_CLASS* that) noexcept             \
    {                                                                \
        return static_cast<DOWN_CLASS*>(that);                       \
    }                                                                \
    inline DOWN_CLASS const* downcast(UP_CLASS const* that) noexcept \
    {                                                                \
        return static_cast<DOWN_CLASS const*>(that);                 \
    }
