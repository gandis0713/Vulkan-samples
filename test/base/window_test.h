#pragma once

#include <SDL2/SDL.h>
#include <gtest/gtest.h>

#include "jipu/native/include/queue.h"
#include "jipu/native/include/surface.h"
#include "jipu/native/include/swapchain.h"
#include "test.h"

namespace jipu
{

class WindowTest : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    void* handle();

    SDL_Window* m_window;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    std::unique_ptr<jipu::Surface> m_surface = nullptr;
    std::unique_ptr<jipu::Queue> m_queue = nullptr;
    std::unique_ptr<jipu::Swapchain> m_swapchain = nullptr;
};

} // namespace jipu