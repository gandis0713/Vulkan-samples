

#include "window.h"

namespace jipu
{

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

bool Window::isInitialized() const
{
    return m_initialized;
}

} // namespace jipu