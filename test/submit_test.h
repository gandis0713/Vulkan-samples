#pragma once
#include "base/window_test.h"

#include "jipu/native/include/texture.h"

namespace jipu
{

class SubmitTest : public WindowTest
{
protected:
    void SetUp() override;
    void TearDown() override;
};

} // namespace jipu