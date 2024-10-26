cmake_minimum_required(VERSION 3.21)

project(
    hpc-backend
    VERSION 0.0.1
    DESCRIPTION "HPC backend"
    LANGUAGES CXX
)

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/instance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/handle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/handle.h

    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_gpu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_gpu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_sampler_a6xx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_sampler_a6xx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_sampler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_instance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/adreno/adreno_instance.h

    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/mali/mali_instance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/device/mali/mali_instance.h

    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/backend/gpu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/backend/instance.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/backend/sampler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/backend/export.h
)

add_library(backend STATIC
    ${SRC_FILES}
)
add_library(hpc::backend ALIAS backend)

target_include_directories(backend
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
)
