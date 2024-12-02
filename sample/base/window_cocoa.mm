#include "window.h"

#include <SDL2/SDL.h>
#include <SDL_syswm.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

namespace jipu
{

void* Window::getWindowHandle()
{
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(static_cast<SDL_Window*>(m_handle), &wmi);

    NSWindow* win = wmi.info.cocoa.window;
    NSView* nsView = [win contentView];
    if (nsView == nil)
    {
        return nullptr;
    }

    NSBundle* bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"];
    CAMetalLayer* layer = [[bundle classNamed:@"CAMetalLayer"] layer];
    [nsView setLayer:layer];
    [nsView setWantsLayer:YES];

    return layer;
}

} // namespace jipu
