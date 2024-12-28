

#include "window.h"

namespace jipu
{

void Window::init()
{
    m_initialized = true;
}

void Window::setWidth(uint32_t width)
{
    m_width = width;
}

void Window::setHeight(uint32_t height)
{
    m_height = height;
}

uint32_t Window::getWidth() const
{
    return m_width;
}

uint32_t Window::getHeight() const
{
    return m_height;
}

uint32_t Window::getWindowWidth() const
{
    return m_windowWidth;
}

uint32_t Window::getWindowHeight() const
{
    return m_windowHeight;
}

} // namespace jipu