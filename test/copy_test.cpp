#include "copy_test.h"

#include <random>

using namespace jipu;

void CopyTest::SetUp()
{
    Test::SetUp();

    // get random value and set image data.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> valueDist(0, 255);
    m_value = static_cast<char>(valueDist(gen));
    m_image = Image{ 256, 256, 4, m_value };
    EXPECT_EQ(m_value, m_image.data[0]);

    // prepare source buffer
    BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = m_image.width * m_image.height * m_image.channel * sizeof(char);
    bufferDescriptor.usage = BufferUsageFlagBits::kCopySrc;

    m_srcBuffer = m_device->createBuffer(bufferDescriptor);
    EXPECT_NE(nullptr, m_srcBuffer);
    void* imageBufferPointer = m_srcBuffer->map();
    EXPECT_NE(nullptr, imageBufferPointer);
    memcpy(imageBufferPointer, m_image.data.data(), bufferDescriptor.size);
    m_srcBuffer->unmap();

    // prepare source texture
    TextureDescriptor textureDescriptor{};
    textureDescriptor.type = TextureType::k2D;
    textureDescriptor.format = TextureFormat::kRGBA8Unorm;
    textureDescriptor.mipLevels = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.width = m_image.width;
    textureDescriptor.height = m_image.height;
    textureDescriptor.depth = 1;
    textureDescriptor.usage = TextureUsageFlagBits::kCopySrc | TextureUsageFlagBits::kCopyDst;

    m_srcTexture = m_device->createTexture(textureDescriptor);
    EXPECT_NE(nullptr, m_srcTexture);

    CopyTextureBuffer copyTextureBuffer{
        .buffer = m_srcBuffer.get(),
        .offset = 0,
        .bytesPerRow = static_cast<uint32_t>(m_image.width * m_image.channel * sizeof(char)),
        .rowsPerTexture = static_cast<uint32_t>(m_image.height),
    };

    CopyTexture copyTexture{
        .texture = m_srcTexture.get(),
        .aspect = TextureAspectFlagBits::kColor,
    };

    Extent3D extent{};
    extent.width = m_image.width;
    extent.height = m_image.height;
    extent.depth = 1;

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);
    EXPECT_NE(nullptr, commandEncoder);

    commandEncoder->copyBufferToTexture(copyTextureBuffer, copyTexture, extent);

    QueueDescriptor queueDescriptor{};

    auto queue = m_device->createQueue(queueDescriptor);
    EXPECT_NE(nullptr, queue);

    auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
    EXPECT_NE(nullptr, commandBuffer);
    queue->submit({ commandBuffer.get() });
}

void CopyTest::TearDown()
{
    m_srcTexture.reset();
    m_srcBuffer.reset();

    Test::TearDown();
}

void CopyTest::copyTextureToBuffer(Texture* srcTexture)
{
    BufferDescriptor dstBufferDescriptor{};
    dstBufferDescriptor.size = m_srcBuffer->getSize(); // TODO: from src texture
    dstBufferDescriptor.usage = BufferUsageFlagBits::kCopyDst;

    auto dstBuffer = m_device->createBuffer(dstBufferDescriptor);
    EXPECT_NE(nullptr, dstBuffer);

    CopyTextureBuffer dstCopyBuffer{
        .buffer = dstBuffer.get(),
        .offset = 0,
        .bytesPerRow = static_cast<uint32_t>(m_image.width * m_image.channel * sizeof(char)),
        .rowsPerTexture = static_cast<uint32_t>(m_image.height),
    };

    CopyTexture srcCopyTexture{
        .texture = srcTexture,
        .aspect = TextureAspectFlagBits::kColor,
    };

    Extent3D extent{};
    extent.depth = 1;
    extent.width = srcTexture->getWidth();
    extent.height = srcTexture->getHeight();

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);
    EXPECT_NE(nullptr, commandEncoder);

    commandEncoder->copyTextureToBuffer(srcCopyTexture, dstCopyBuffer, extent);

    QueueDescriptor queueDescriptor{};

    auto queue = m_device->createQueue(queueDescriptor);
    EXPECT_NE(nullptr, queue);
    auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
    EXPECT_NE(nullptr, commandBuffer);
    queue->submit({ commandBuffer.get() });

    char* dstBufferPointer = static_cast<char*>(dstBuffer->map());
    char firstData = *dstBufferPointer;
    EXPECT_EQ(firstData, m_value);
}

TEST_F(CopyTest, test_BufferToBuffer)
{

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);
    EXPECT_NE(nullptr, commandEncoder);

    CopyBuffer srcCopyBuffer{
        .buffer = m_srcBuffer.get(),
        .offset = 0,
    };

    BufferDescriptor bufferDescriptor{};
    bufferDescriptor.size = m_srcBuffer->getSize();
    bufferDescriptor.usage = BufferUsageFlagBits::kCopyDst;

    auto buffer = m_device->createBuffer(bufferDescriptor);
    EXPECT_NE(nullptr, buffer);

    CopyBuffer dstCopyBuffer{
        .buffer = buffer.get(),
        .offset = 0,
    };

    commandEncoder->copyBufferToBuffer(srcCopyBuffer, dstCopyBuffer, m_srcBuffer->getSize());

    QueueDescriptor queueDescriptor{};

    auto queue = m_device->createQueue(queueDescriptor);
    EXPECT_NE(nullptr, queue);
    auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
    EXPECT_NE(nullptr, commandBuffer);
    queue->submit({ commandBuffer.get() });

    void* dstBufferPointer = buffer->map();
    EXPECT_NE(nullptr, dstBufferPointer);

    char* dataPointer = static_cast<char*>(dstBufferPointer);
    char firstData = *dataPointer;
    EXPECT_EQ(firstData, m_value);
}

TEST_F(CopyTest, test_BufferToTexture)
{
    TextureDescriptor textureDescriptor{};
    textureDescriptor.type = TextureType::k2D;
    textureDescriptor.format = TextureFormat::kRGBA8Unorm;
    textureDescriptor.mipLevels = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.width = m_image.width;
    textureDescriptor.height = m_image.height;
    textureDescriptor.depth = 1;
    textureDescriptor.usage = TextureUsageFlagBits::kCopySrc | // to check copied data by copying buffer.
                              TextureUsageFlagBits::kCopyDst;

    auto texture = m_device->createTexture(textureDescriptor);
    EXPECT_NE(nullptr, texture);

    CopyTextureBuffer copyTextureBuffer{
        .buffer = m_srcBuffer.get(),
        .offset = 0,
        .bytesPerRow = static_cast<uint32_t>(m_image.width * m_image.channel * sizeof(char)),
        .rowsPerTexture = static_cast<uint32_t>(m_image.height),
    };

    CopyTexture copyTexture{
        .texture = texture.get(),
        .aspect = TextureAspectFlagBits::kColor,
    };

    Extent3D extent{};
    extent.width = m_image.width;
    extent.height = m_image.height;
    extent.depth = 1;

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);
    EXPECT_NE(nullptr, commandEncoder);

    commandEncoder->copyBufferToTexture(copyTextureBuffer, copyTexture, extent);

    QueueDescriptor queueDescriptor{};

    auto queue = m_device->createQueue(queueDescriptor);
    EXPECT_NE(nullptr, queue);
    auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
    EXPECT_NE(nullptr, commandBuffer);
    queue->submit({ commandBuffer.get() });

    copyTextureToBuffer(texture.get()); // to check copied texture data.
}

TEST_F(CopyTest, test_TextureToBuffer)
{
    copyTextureToBuffer(m_srcTexture.get());
}

TEST_F(CopyTest, test_TextureToTexture)
{
    TextureDescriptor textureDescriptor{};
    textureDescriptor.type = TextureType::k2D;
    textureDescriptor.format = TextureFormat::kRGBA8Unorm;
    textureDescriptor.mipLevels = 1;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.width = m_image.width;
    textureDescriptor.height = m_image.height;
    textureDescriptor.depth = 1;
    textureDescriptor.usage = TextureUsageFlagBits::kCopySrc | // to check copied data by copying buffer.
                              TextureUsageFlagBits::kCopyDst;

    auto dstTexture = m_device->createTexture(textureDescriptor);
    EXPECT_NE(nullptr, dstTexture);

    CopyTexture srcCopyTexture{
        .texture = m_srcTexture.get(),
        .aspect = TextureAspectFlagBits::kColor,
    };

    CopyTexture dstCopyTexture{
        .texture = dstTexture.get(),
        .aspect = TextureAspectFlagBits::kColor,
    };

    Extent3D extent{};
    extent.depth = 1;
    extent.width = m_srcTexture->getWidth();
    extent.height = m_srcTexture->getHeight();

    CommandEncoderDescriptor commandEncoderDescriptor{};
    auto commandEncoder = m_device->createCommandEncoder(commandEncoderDescriptor);
    EXPECT_NE(nullptr, commandEncoder);

    commandEncoder->copyTextureToTexture(srcCopyTexture, dstCopyTexture, extent);

    QueueDescriptor queueDescriptor{};

    auto queue = m_device->createQueue(queueDescriptor);
    EXPECT_NE(nullptr, queue);
    auto commandBuffer = commandEncoder->finish(CommandBufferDescriptor{});
    EXPECT_NE(nullptr, commandBuffer);
    queue->submit({ commandBuffer.get() });

    copyTextureToBuffer(dstTexture.get()); // to check copied texture data.
}