#include "window.h"

#include <SDL2/SDL.h>
#include <SDL_syswm.h>
#include <spdlog/spdlog.h>

namespace jipu
{

Window::Window(const WindowDescriptor& descriptor)
    : m_handle(descriptor.handle)
    , m_width(descriptor.width)
    , m_height(descriptor.height)
    , m_windowWidth(descriptor.width)
    , m_windowHeight(descriptor.height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return;
    }

    SDL_Window* window = SDL_CreateWindow(descriptor.title.c_str(),
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          m_windowWidth,
                                          m_windowHeight,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI); // | SDL_WINDOW_RESIZABLE

    int drawableWidth, drawableHeight;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

    float devicePixelRatio = (float)drawableWidth / (float)m_windowWidth;

    m_width = m_windowWidth * devicePixelRatio;
    m_height = m_windowHeight * devicePixelRatio;

    if (!window)
    {
        SDL_Quit();
        return;
    }

    m_handle = static_cast<void*>(window);
}

Window::~Window()
{
    SDL_DestroyWindow(static_cast<SDL_Window*>(m_handle));
    SDL_Quit();
}

int Window::exec()
{
    init();

    SDL_Event event;
    int quit = 0;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.state == SDL_PRESSED)
                {
                    m_leftMouseButton = (event.button.button == SDL_BUTTON_LEFT);
                    m_rightMouseButton = (event.button.button == SDL_BUTTON_RIGHT);
                    m_middleMouseButton = (event.button.button == SDL_BUTTON_MIDDLE);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.state == SDL_RELEASED)
                {
                    m_leftMouseButton = m_rightMouseButton = m_middleMouseButton = false;
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    m_width = event.window.data1;
                    m_height = event.window.data2;
                    onResize(m_width, m_height);
                }
                break;
            default:
                // do nothing.
                break;
            }

            SDL_GetMouseState(&m_mouseX, &m_mouseY);

            // spdlog::trace("mouse button l: {}, r: {}, m: {}", m_leftMouseButton, m_rightMouseButton, m_middleMouseButton);
            // spdlog::trace("mouse x: {}, y: {}", m_mouseX, m_mouseY);
        }

        onUpdate();
        onDraw();
    }

    return 0;
}

} // namespace jipu