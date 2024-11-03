cmake_minimum_required(VERSION 3.22)

# find package
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# find_library(VulkanSDK NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib")
find_package(spdlog CONFIG REQUIRED)

set(NATIVE_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_api.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_bind_group_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_bind_group.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_recorder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_buffer_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_compute_pass_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_inflight_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_instance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_fence_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_framebuffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_resource_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_resource_synchronizer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_resource_tracker.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_pipeline.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_pipeline_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_physical_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_query_set.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_render_pass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_render_pass_encoder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_sampler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_shader_module.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_semaphore_pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_submit_context.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_swapchain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_texture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_texture_view.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_api.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_bind_group_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_bind_group.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_recorder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_buffer_tracker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_compute_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_inflight_context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_instance.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_fence_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_framebuffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_resource.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_resource_allocator.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_resource_synchronizer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_command_resource_tracker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_pipeline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_pipeline_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_physical_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_query_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_render_pass.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_render_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_sampler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_shader_module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_semaphore_pool.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_submit_context.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_surface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_swapchain.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_texture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_texture_view.h

  ${CMAKE_CURRENT_SOURCE_DIR}/instance.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/bind_group.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pipeline_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/command_buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/command_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/compute_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/texture.h
  ${CMAKE_CURRENT_SOURCE_DIR}/device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/pipeline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/query_set.h
  ${CMAKE_CURRENT_SOURCE_DIR}/buffer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/swapchain.h
  ${CMAKE_CURRENT_SOURCE_DIR}/instance.h
  ${CMAKE_CURRENT_SOURCE_DIR}/queue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/render_pass_encoder.h
  ${CMAKE_CURRENT_SOURCE_DIR}/texture_view.h
  ${CMAKE_CURRENT_SOURCE_DIR}/physical_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sampler.h
  ${CMAKE_CURRENT_SOURCE_DIR}/surface.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shader_module.h

  ${CMAKE_CURRENT_SOURCE_DIR}/export.h
)

if(APPLE)
  list(APPEND NATIVE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_surface_macos.mm
  )
elseif(MSVC)
  list(APPEND NATIVE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_surface_window.cpp
  )
elseif(ANDROID)
  list(APPEND NATIVE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_surface_android.cpp
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

# include directories
target_include_directories(jipu_native
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vulkan>
)

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
  "VULKAN_IMPLEMENTATION"
)

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

if(EXPORT_JIPU_NATIVE)
  message(STATUS "jipu::native will be installed")

  target_include_directories(jipu_native
    PUBLIC
    $<INSTALL_INTERFACE:include>
  )

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/jipu/native
    FILES_MATCHING PATTERN "*.h"
  )

  # install(TARGETS jipu_native
  # EXPORT native_targets
  # RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  # LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  # ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  # )

  # install(EXPORT jipu_native_targets
  # FILE native-config.cmake
  # NAMESPACE jipu::
  # DESTINATION ${CMAKE_INSTALL_PREFIX}/share/jipu
  # )
endif()
