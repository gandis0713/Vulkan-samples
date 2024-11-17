cmake_minimum_required(VERSION 3.22)

find_package(spdlog REQUIRED)

set(COMMON_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/dylib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gpu_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ref_counted.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/assert.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cast.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dylib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fmt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gpu_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ref_counted.h
    ${CMAKE_CURRENT_SOURCE_DIR}/result.h
    ${CMAKE_CURRENT_SOURCE_DIR}/thread_pool.h
)

add_library(common
    STATIC # SHARED or STATIC
    ${COMMON_SRC_FILES}
)

add_library(jipu::common ALIAS common)

# include directories
target_include_directories(common
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_link_libraries(common
    PRIVATE
    spdlog::spdlog_header_only
)

if(EXPORT_JIPU_COMMON)
    message(STATUS "jipu::common will be installed")

    target_include_directories(common
        PUBLIC
        $<INSTALL_INTERFACE:include>
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/jipu/common
        FILES_MATCHING PATTERN "*.h"
    )

    # install(TARGETS common
    # EXPORT common_targets
    # RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    # LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    # ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    # )

    # install(EXPORT common_targets
    # FILE common-config.cmake
    # NAMESPACE jipu::
    # DESTINATION ${CMAKE_INSTALL_PREFIX}/share/jipu
    # )
endif()