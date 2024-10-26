#include "texture_test.h"

#include "jipu/native/include/texture.h"

using namespace jipu;

TEST_F(TextureTest, test_createtexture_usage)
{
    std::unique_ptr<Texture> texture = nullptr;

    TextureDescriptor descriptor{};
    descriptor.width = 1;
    descriptor.height = 1;
    descriptor.depth = 1;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = 1;
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kRGBA8Unorm;

    {
        descriptor.usage = TextureUsageFlagBits::kUndefined;
        ASSERT_ANY_THROW({ m_device->createTexture(descriptor); });
    }

    {
        descriptor.usage = TextureUsageFlagBits::kColorAttachment;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.usage = TextureUsageFlagBits::kCopyDst;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.usage = TextureUsageFlagBits::kCopySrc;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.usage = TextureUsageFlagBits::kDepthStencil;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.usage = TextureUsageFlagBits::kStorageBinding;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.usage = TextureUsageFlagBits::kTextureBinding;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }
}

TEST_F(TextureTest, test_createtexture_with_size)
{
    std::unique_ptr<Texture> texture = nullptr;

    TextureDescriptor descriptor{};
    descriptor.mipLevels = 1;
    descriptor.sampleCount = 1;
    descriptor.type = TextureType::k2D;
    descriptor.format = TextureFormat::kRGBA8Unorm;
    descriptor.usage = TextureUsageFlagBits::kTextureBinding;

    {
        descriptor.width = 0;
        descriptor.height = 1;
        descriptor.depth = 1;
        ASSERT_ANY_THROW({ m_device->createTexture(descriptor); });
    }

    {
        descriptor.width = 1;
        descriptor.height = 0;
        descriptor.depth = 1;
        ASSERT_ANY_THROW({ m_device->createTexture(descriptor); });
    }

    {
        descriptor.width = 1;
        descriptor.height = 1;
        descriptor.depth = 0;
        ASSERT_ANY_THROW({ m_device->createTexture(descriptor); });
    }
}

TEST_F(TextureTest, test_createtexture_with_type)
{
    std::unique_ptr<Texture> texture = nullptr;

    TextureDescriptor descriptor{};
    descriptor.width = 1;
    descriptor.height = 1;
    descriptor.depth = 1;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = 1;
    descriptor.format = TextureFormat::kRGBA8Unorm;
    descriptor.usage = TextureUsageFlagBits::kTextureBinding;

    {
        descriptor.type = TextureType::kUndefined;
        ASSERT_ANY_THROW({ m_device->createTexture(descriptor); });
    }

    {
        descriptor.type = TextureType::k1D;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.type = TextureType::k2D;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.type = TextureType::k3D;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }
}

TEST_F(TextureTest, test_createtexture_with_format)
{
    std::unique_ptr<Texture> texture = nullptr;

    TextureDescriptor descriptor{};
    descriptor.width = 1;
    descriptor.height = 1;
    descriptor.depth = 1;
    descriptor.mipLevels = 1;
    descriptor.sampleCount = 1;
    descriptor.type = TextureType::k2D;
    descriptor.usage = TextureUsageFlagBits::kTextureBinding;

    {
        descriptor.format = TextureFormat::kUndefined;
        ASSERT_ANY_THROW({ m_device->createTexture(descriptor); });
    }

    {
        descriptor.format = TextureFormat::kBGRA8Unorm;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.format = TextureFormat::kBGRA8UnormSrgb;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        // descriptor.format = TextureFormat::kD_24_UInt_Norm_S_8_UInt;
        // texture = m_device->createTexture(descriptor);
        // ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.format = TextureFormat::kDepth32Float;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        // descriptor.format = TextureFormat::kRGB_888_UInt_Norm;
        // texture = m_device->createTexture(descriptor);
        // ASSERT_NE(texture, nullptr);
    }

    {
        // descriptor.format = TextureFormat::kRGB_888_UInt_Norm_SRGB;
        // texture = m_device->createTexture(descriptor);
        // ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.format = TextureFormat::kRGBA16Unorm;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.format = TextureFormat::kRGBA8Unorm;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }

    {
        descriptor.format = TextureFormat::kRGBA8UnormSrgb;
        texture = m_device->createTexture(descriptor);
        ASSERT_NE(texture, nullptr);
    }
}
