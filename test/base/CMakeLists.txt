cmake_minimum_required(VERSION 3.22)

find_package(Gtest CONFIG REQUIRED)
find_package(sdl2 CONFIG REQUIRED)

set(PRJ_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test.h
  ${CMAKE_CURRENT_SOURCE_DIR}/window_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/window_test.h
)

if(APPLE)
  list(APPEND PRJ_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/window_test_cocoa.mm
  )
elseif(WIN32)
  list(APPEND PRJ_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/window_test_win32.cpp
  )
endif()

add_library(test_base STATIC ${PRJ_SRCS})
add_library(jipu::test_base ALIAS test_base)
target_include_directories(test_base PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_base
  PUBLIC
  jipu::webgpu
  GTest::gmock
  GTest::gtest
  GTest::gmock_main
  GTest::gtest_main
  SDL2::SDL2
  SDL2::SDL2main
)