cmake_minimum_required(VERSION 3.22)

# find header library
find_path(WEBGPU_HEADERS_INCLUDE_DIRS
  NAMES "webgpu.h"
  HINTS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include
  REQUIRED
)
find_package(Dawn CONFIG REQUIRED)

set(WEBGPU_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_adapter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_adapter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_bind_group_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_bind_group_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_bind_group.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_bind_group.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_command_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_command_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_command_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_compute_pass_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_compute_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_compute_pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_compute_pipeline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_instance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_instance.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_pipeline_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_pipeline_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_bundle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_bundle.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_bundle_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_bundle_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_pass_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_render_pipeline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_sampler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_sampler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_shader_module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_shader_module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_surface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_texture_view.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_texture_view.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_texture.h

  ${CMAKE_CURRENT_SOURCE_DIR}/event/event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event/event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/event/event_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event/event_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/event/request_adapter_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event/request_adapter_event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/event/request_device_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event/request_device_event.h
  ${CMAKE_CURRENT_SOURCE_DIR}/event/queue_work_done_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event/queue_work_done_event.h
)

set(LIB_TYPE STATIC)

if(BUILD_SHARED_LIBS)
  set(LIB_TYPE SHARED)
endif()

add_library(jipu_webgpu
  ${LIB_TYPE}
  ${WEBGPU_SRC_FILES}
)
add_library(jipu::webgpu ALIAS jipu_webgpu)

# include directories
target_include_directories(jipu_webgpu
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
  $<BUILD_INTERFACE:${WEBGPU_HEADERS_INCLUDE_DIRS}>
)

# link libraries
target_link_libraries(jipu_webgpu
  PUBLIC
  jipu::native
  dawn::webgpu_dawn
)

# compile options
if(MSVC)
# TODO
elseif(APPLE)
  target_compile_options(jipu_webgpu
    PRIVATE
    -Wall
    -Werror
    -Wshorten-64-to-32
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-unused-but-set-variable
    -Wno-unused-variable

    -fvisibility=hidden
    -fvisibility-inlines-hidden

    -fno-rtti # -fno-exceptions
  )
endif()

target_compile_definitions(jipu_webgpu
  PRIVATE
  "WGPU_IMPLEMENTATION"
)

if(USE_DAWN_WEBGPU)
  target_compile_definitions(jipu_webgpu
    PUBLIC
    "USE_DAWN_HEADER"
  )
endif()

if(BUILD_SHARED_LIBS)
  target_compile_definitions(jipu_webgpu
    PRIVATE
    "WGPU_SHARED_LIBRARY"
  )
endif()
