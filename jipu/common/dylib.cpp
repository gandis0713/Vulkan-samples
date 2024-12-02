#include "dylib.h"

#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#elif defined(WIN32)
// TODO: other header include
#include <windows.h>
#endif
#include <spdlog/spdlog.h>

namespace jipu
{

DyLib::~DyLib()
{
    close();
}

DyLib::DyLib(DyLib&& rhs)
{
    std::swap(m_handle, rhs.m_handle);
}

DyLib& DyLib::operator=(DyLib&& rhs)
{
    std::swap(m_handle, rhs.m_handle);
    return *this;
}

bool DyLib::isValid() const
{
    return m_handle != nullptr;
}

bool DyLib::open(const std::string& filename)
{
#if defined(__linux__) || defined(__APPLE__)
    m_handle = dlopen(filename.c_str(), RTLD_NOW | RTLD_LOCAL);

    if (m_handle == nullptr)
    {
        spdlog::error(dlerror());
    }
#elif defined(WIN32)
    m_handle = LoadLibraryA(filename.c_str());

    if (m_handle == nullptr)
    {
        spdlog::error("Windows Error: {}" + std::to_string(GetLastError()));
    }
#else
    spdlog::error("Unsupported platform for DyLib");
#endif

    return m_handle != nullptr;
}

void DyLib::close()
{
    if (m_handle == nullptr)
    {
        return;
    }

#if defined(__linux__) || defined(__APPLE__)
    dlclose(m_handle);
#elif defined(WIN32)
    FreeLibrary(static_cast<HMODULE>(m_handle));
#else
    spdlog::error("Unsupported platform for DyLib");
#endif

    m_handle = nullptr;
}

void* DyLib::getProc(const std::string& procName) const
{
    void* proc = nullptr;

#if defined(__linux__) || defined(__APPLE__)
    proc = reinterpret_cast<void*>(dlsym(m_handle, procName.c_str()));

    if (proc == nullptr)
    {
        spdlog::error(dlerror());
    }
#elif defined(WIN32)
    proc = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(m_handle), procName.c_str()));

    if (proc == nullptr)
    {
        spdlog::error("Windows Error: {}" + std::to_string(GetLastError()));
    }
#else
    spdlog::error("Unsupported platform for DyLib");
#endif

    return proc;
}

} // namespace jipu