
#include "test.h"

namespace jipu
{

void Test::SetUp()
{
    InstanceDescriptor instanceDescriptor;
    m_instance = Instance::create(instanceDescriptor);
    EXPECT_NE(nullptr, m_instance);

    AdapterDescriptor adapterDescriptor;
    adapterDescriptor.type = BackendAPI::kVulkan;
    m_adapter = m_instance->createAdapter(adapterDescriptor);
    EXPECT_NE(nullptr, m_adapter);

    m_physicalDevices = m_adapter->getPhysicalDevices();
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