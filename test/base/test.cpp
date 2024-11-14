
#include "test.h"

namespace jipu
{

void Test::SetUp()
{
    AdapterDescriptor instanceDescriptor;
    instanceDescriptor.type = AdapterType::kVulkan;
    m_instance = Adapter::create(instanceDescriptor);
    EXPECT_NE(nullptr, m_instance);

    m_physicalDevices = m_instance->getPhysicalDevices();
    EXPECT_NE(0, m_physicalDevices.size());

    PhysicalDevice* physicalDevice = m_physicalDevices[0].get();
    DeviceDescriptor deviceDescriptor{};
    m_device = physicalDevice->createDevice(deviceDescriptor);
    EXPECT_NE(nullptr, m_device);
}

void Test::TearDown()
{
    m_device.reset();
    m_physicalDevices.clear();
    m_instance.reset();
}

} // namespace jipu