#pragma once

// clang-format off
#if defined(JIPU_SHARED_LIBRARY)
    #if defined(_WIN32)
        #if defined(JIPU_IMPLEMENTATION)
            #define JIPU_EXPORT __declspec(dllexport)
        #else
            #define JIPU_EXPORT __declspec(dllimport)
        #endif
    #else // defined(_WIN32)
        #if defined(JIPU_IMPLEMENTATION)
            #define JIPU_EXPORT __attribute__((visibility("default")))
        #else
            #define JIPU_EXPORT
        #endif
    #endif // defined(_WIN32)
#else      // defined(JIPU_SHARED_LIBRARY)
    #define JIPU_EXPORT
#endif // defined(JIPU_SHARED_LIBRARY)
// clang-format on