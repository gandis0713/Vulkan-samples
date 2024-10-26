#pragma once

#include "assert.h"

#include <string>
#include <type_traits>

namespace jipu
{

class DyLib
{
public:
    DyLib() = default;
    ~DyLib();

    DyLib(const DyLib&) = delete;
    DyLib& operator=(const DyLib&) = delete;

    DyLib(DyLib&& other);
    DyLib& operator=(DyLib&& other);

    bool isValid() const;

    bool open(const std::string& filename);
    void close();

    void* getProc(const std::string& procName) const;

    template <typename T>
    bool getProc(T** proc, const std::string& procName) const
    {
        assert_message(proc != nullptr, "proc is null pointer.");
        static_assert(std::is_function<T>::value, "");

        *proc = reinterpret_cast<T*>(getProc(procName));
        return *proc != nullptr;
    }

private:
    void* m_handle = nullptr;
};
} // namespace jipu