#include "sample.h"

#include <stdexcept>

namespace jipu
{

Sample::Sample(const SampleDescriptor& descriptor)
    : Window(descriptor.windowDescriptor)
    , m_appPath(descriptor.path)
    , m_appDir(descriptor.path.parent_path())
{
}

Sample::~Sample()
{
}

} // namespace jipu