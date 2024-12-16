#pragma once

#define WGPU_SKIP_DECLARATIONS // TODO: remove this after publishing the header

#if defined(USE_DAWN_HEADER)
#include <dawn/webgpu.h>

using WGPUTextureUsageFlags = WGPUTextureUsage;

#else
#include <webgpu.h>
#endif