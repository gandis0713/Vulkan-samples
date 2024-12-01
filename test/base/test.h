
#pragma once

#include <SDL2/SDL.h>
#include <gtest/gtest.h>

#include "jipu/native/adapter.h"
#include "jipu/native/device.h"
#include "jipu/native/instance.h"
#include "jipu/native/physical_device.h"

namespace jipu
{

class Test : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<jipu::Instance> m_instance = nullptr;
    std::unique_ptr<jipu::Adapter> m_adapter = nullptr;
    std::vector<std::unique_ptr<jipu::PhysicalDevice>> m_physicalDevices{};
    std::unique_ptr<jipu::Device> m_device = nullptr;
};

} // namespace jipu
