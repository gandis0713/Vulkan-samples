
#pragma once

#include <SDL2/SDL.h>
#include <gtest/gtest.h>

#include "jipu/device.h"
#include "jipu/instance.h"
#include "jipu/physical_device.h"

namespace jipu
{

class Test : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<jipu::Instance> m_instance = nullptr;
    std::vector<std::unique_ptr<jipu::PhysicalDevice>> m_physicalDevices{};
    std::unique_ptr<jipu::Device> m_device = nullptr;
};

} // namespace jipu
