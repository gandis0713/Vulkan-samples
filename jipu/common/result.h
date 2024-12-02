#pragma once

#include <memory>
#include <variant>

namespace jipu
{

template <class T, class E>
class Result
{
public:
    explicit Result(T value)
        : result(std::move(value)) {};
    explicit Result(E error)
        : result(error) {};

    bool isOk() const
    {
        return std::holds_alternative<T>(result);
    };

    bool isError() const
    {
        return !isOk();
    }

    T unwrap()
    {
        return std::get<T>(std::move(result));
    }
    E unwrapError()
    {
        return std::get<E>(result);
    }

private:
    std::variant<T, E> result;
};

} // namespace jipu