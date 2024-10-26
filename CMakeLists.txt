cmake_minimum_required(VERSION 3.21)

# set options.
option(JIPU_TEST "JIPU Test" OFF)
option(JIPU_SAMPLE "JIPU Sample" ON)

# TODO: move cmake/ directory.
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
    if(ANDROID_ABI MATCHES "arm64-v8a")
      set(VCPKG_TARGET_TRIPLET "arm64-android")
    elseif(ANDROID_ABI MATCHES "armeabi-v7a")
      set(VCPKG_TARGET_TRIPLET "arm-neon-android")
    elseif(ANDROID_ABI MATCHES "x86_64")
      set(VCPKG_TARGET_TRIPLET "x64-android")
    elseif(ANDROID_ABI MATCHES "x86")
      set(VCPKG_TARGET_TRIPLET "x86-android")
    endif()
  endif()
endif()

project(
  jipu
  VERSION 0.0.1
  DESCRIPTION "Abstract modern graphics API"
  LANGUAGES CXX
)

# set compile options.
set(CMAKE_CXX_STANDARD 20)

if(MSVC)
  add_compile_options(
    /MP
  )
elseif(APPLE)
  enable_language(OBJCXX)
endif()

add_subdirectory(jipu)

if(JIPU_SAMPLE)
  add_subdirectory(sample)
endif()

if(JIPU_TEST)
  enable_testing()
  add_subdirectory(test)

  # add_dependencies(test_jipu jipu::native)
endif()
