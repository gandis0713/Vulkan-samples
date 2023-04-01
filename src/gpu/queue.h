#pragma once

namespace vkt
{

enum class QueueType
{
    kGraphics = 0,
    kCompute,
};

struct QueueDescriptor
{
    QueueType type;
};

class Device;
class Queue
{
public:
    Queue() = delete;
    Queue(Device* device, const QueueDescriptor& descriptor);
    virtual ~Queue() = default;

    virtual QueueType getType() const = 0;

protected:
    Device* m_device;
};

} // namespace vkt