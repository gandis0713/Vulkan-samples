#pragma once

#if defined(USE_DAWN_HEADER)
#include <dawn/webgpu.h>

using WGPUTextureUsageFlags = WGPUTextureUsage;

#else
#include <webgpu.h>
#endif