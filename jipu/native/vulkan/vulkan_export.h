#pragma once

// clang-format off
#if defined(JIPU_SHARED_LIBRARY)
    #if defined(_WIN32)
        #if defined(VULKAN_IMPLEMENTATION)
            #define VULKAN_EXPORT __declspec(dllexport)
        #else
            #define VULKAN_EXPORT __declspec(dllimport)
        #endif
    #else // defined(_WIN32)
        #if defined(VULKAN_IMPLEMENTATION)
            #define VULKAN_EXPORT __attribute__((visibility("default")))
        #else
            #define VULKAN_EXPORT
        #endif
    #endif // defined(_WIN32)
#else      // defined(JIPU_SHARED_LIBRARY)
    #define VULKAN_EXPORT
#endif // defined(JIPU_SHARED_LIBRARY)
// clang-format on