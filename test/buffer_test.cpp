#include "buffer_test.h"

#include "jipu/native/include/buffer.h"

using namespace jipu;

TEST_F(BufferTest, test_createbuffer_usage)
{
    std::unique_ptr<Buffer> buffer = nullptr;

    BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = 1;

    {
        bufferDescriptor.usage = BufferUsageFlagBits::kUndefined;
        ASSERT_ANY_THROW({ m_device->createBuffer(bufferDescriptor); });
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kMapRead;
        // ASSERT_ANY_THROW({ m_device->createBuffer(bufferDescriptor); }); // TODO: implement
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kMapWrite;
        // ASSERT_ANY_THROW({ m_device->createBuffer(bufferDescriptor); }); // TODO: implement
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kIndex;
        buffer = m_device->createBuffer(bufferDescriptor);
        ASSERT_NE(buffer, nullptr);
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kVertex;
        buffer = m_device->createBuffer(bufferDescriptor);
        ASSERT_NE(buffer, nullptr);
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kUniform;
        buffer = m_device->createBuffer(bufferDescriptor);
        ASSERT_NE(buffer, nullptr);
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kStorage;
        buffer = m_device->createBuffer(bufferDescriptor);
        ASSERT_NE(buffer, nullptr);
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kCopySrc;
        buffer = m_device->createBuffer(bufferDescriptor);
        ASSERT_NE(buffer, nullptr);
    }
    {
        bufferDescriptor.usage = BufferUsageFlagBits::kCopyDst;
        buffer = m_device->createBuffer(bufferDescriptor);
        ASSERT_NE(buffer, nullptr);
    }
}

TEST_F(BufferTest, test_createbuffer_with_size)
{
    BufferDescriptor bufferDescriptor{};
    bufferDescriptor.usage = BufferUsageFlagBits::kVertex;

    {
        bufferDescriptor.size = 0;
        ASSERT_ANY_THROW({ m_device->createBuffer(bufferDescriptor); });
    }

    {
        bufferDescriptor.size = 1;
        auto buffer = m_device->createBuffer(bufferDescriptor);
        ASSERT_NE(buffer, nullptr);
    }
}