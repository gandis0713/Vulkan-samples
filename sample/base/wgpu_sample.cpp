#include "wgpu_sample.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

WGPUSample::WGPUSample(const WGPUSampleDescriptor& descriptor)
    : Window(descriptor.windowDescriptor)
    , m_appPath(descriptor.path)
    , m_appDir(descriptor.path.parent_path())
{
}

WGPUSample::~WGPUSample()
{
}

void WGPUSample::init()
{
    Window::init();
}

void WGPUSample::update()
{
}

} // namespace jipu