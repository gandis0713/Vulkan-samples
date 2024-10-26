#pragma once
#include "base/test.h"

#include "jipu/native/texture.h"

namespace jipu
{

class CopyTest : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

protected:
    void copyTextureToBuffer(Texture* dstTexture);

protected:
    std::unique_ptr<Buffer> m_srcBuffer = nullptr;
    std::unique_ptr<Texture> m_srcTexture = nullptr;
    char m_value = 0x00;
    struct Image
    {
        Image(int w = 256, int h = 256, int c = 4, char d = 0x00)
            : width(w)
            , height(h)
            , channel(c)
            , data(w * h * c, d)
        {
        }
        int width = 0;
        int height = 0;
        int channel = 0;
        std::vector<char> data{};
    } m_image;
};

} // namespace jipu