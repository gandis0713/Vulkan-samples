cmake_minimum_required(VERSION 3.21)

if(ANDROID)
    find_package(hwcpipe CONFIG REQUIRED)
endif()

find_package(spdlog CONFIG REQUIRED)

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/instance.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/sampler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/instance.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/gpu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/counter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/hpc/export.h
)

if(ANDROID)
    list(APPEND SRC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_sampler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_sampler.h
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_expression.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_expression.h
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_instance.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_instance.h
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_gpu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_gpu.h
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_counter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/adreno/adreno_counter.h
    )

    list(APPEND SRC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_sampler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_sampler.h
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_instance.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_instance.h
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_gpu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_gpu.h
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_counter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/source/mali/mali_counter.h
    )
endif()

add_library(hpc STATIC ${SRC_FILES})
add_library(hpc::hpc ALIAS hpc)

target_include_directories(
    hpc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
)

if(ANDROID)
    target_include_directories(
        hpc
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/adreno>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/mali>
    )
endif()

if(ANDROID)
    # for custom backend
    target_link_libraries(hpc PRIVATE
        hpc::backend
    )

    # for HWCPipe
    target_link_libraries(hpc PRIVATE
        hwcpipe::device
        hwcpipe::hwcpipe
        hwcpipe::device_private
    )
endif()