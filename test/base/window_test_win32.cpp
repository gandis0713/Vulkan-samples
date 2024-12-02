#include "window_test.h"

#include <SDL_syswm.h>

namespace jipu
{

void* WindowTest::handle()
{
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(static_cast<SDL_Window*>(m_window), &wmi);
    return static_cast<void*>(wmi.info.win.window);
}

} // namespace jipu