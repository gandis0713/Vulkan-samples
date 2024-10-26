cmake_minimum_required(VERSION 3.22)

# find package
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# find_library(VulkanSDK NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib")
find_package(spdlog CONFIG REQUIRED)

set(NATIVE_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_api.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_binding_group_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_binding_group.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_recorder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_buffer_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_compute_pass_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_inflight_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_instance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_fence_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_framebuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_resource_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_resource_synchronizer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_resource_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_pipeline_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_physical_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_query_set.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_render_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_render_pass_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_sampler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_shader_module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_semaphore_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_submit_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_swapchain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_texture_view.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_api.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_binding_group_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_binding_group.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_recorder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_buffer_tracker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_compute_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_inflight_context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_instance.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_fence_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_framebuffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_resource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_resource_allocator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_resource_synchronizer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_command_resource_tracker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_pipeline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_pipeline_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_physical_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_query_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_render_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_render_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_sampler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_shader_module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_semaphore_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_submit_context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_surface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_swapchain.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_texture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_texture_view.h

  ${CMAKE_CURRENT_SOURCE_DIR}/source/jipu/instance.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/include/binding_group.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/pipeline_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/command_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/compute_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/texture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/pipeline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/query_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/swapchain.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/instance.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/render_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/texture_view.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/physical_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/sampler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/surface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/shader_module.h

  ${CMAKE_CURRENT_SOURCE_DIR}/include/export.h
)

if(APPLE)
  list(APPEND NATIVE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_surface_macos.mm
  )
elseif(MSVC)
  list(APPEND NATIVE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_surface_window.cpp
  )
elseif(ANDROID)
  list(APPEND NATIVE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan/vulkan_surface_android.cpp
  )
endif()

set(LIB_TYPE STATIC)

if(BUILD_SHARED_LIBS)
  set(LIB_TYPE SHARED)
endif()

add_library(jipu_native
  ${LIB_TYPE} # SHARED or STATIC
  ${NATIVE_SRC_FILES}
)
add_library(jipu::native ALIAS jipu_native)

target_include_directories(jipu_native
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# include directories
if(EXPORT_JIPU_NATIVE)
  target_include_directories(jipu_native
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<INSTALL_INTERFACE:include>
  )
endif()

# link libraries
target_link_libraries(jipu_native
  PUBLIC
  jipu::common
  PRIVATE
  Vulkan::Headers
  GPUOpen::VulkanMemoryAllocator
)

if(APPLE)
  target_link_libraries(jipu_native
    PRIVATE
    "-framework Cocoa"
  )
endif()

# compile options
if(MSVC)
# TODO
elseif(APPLE)
  target_compile_options(jipu_native
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

target_compile_definitions(jipu_native
  PRIVATE
  "JIPU_IMPLEMENTATION"
)

if(EXPORT_JIPU_VULKAN)
  target_compile_definitions(jipu_native
    PRIVATE
    "VULKAN_IMPLEMENTATION"
  )
endif()

if(BUILD_SHARED_LIBS)
  target_compile_definitions(jipu_native
    PRIVATE
    "JIPU_SHARED_LIBRARY"
  )
endif()

if(MSVC)
  target_compile_definitions(jipu_native
    PRIVATE
    "NOMINMAX" # to undefine max, min macro
  )
endif()

install(TARGETS jipu_native
  EXPORT jipu_native_targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
  DESTINATION include/jipu/native
)

if(EXPORT_JIPU_VULKAN)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/source/vulkan
    DESTINATION include/jipu/native/vulkan
    FILES_MATCHING PATTERN "*.h"
  )
endif()

install(EXPORT jipu_native_targets
  FILE jipu_native-config.cmake
  NAMESPACE jipu::
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/jipu_native
)
