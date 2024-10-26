cmake_minimum_required(VERSION 3.22)

# find header library
find_path(WEBGPU_HEADERS_INCLUDE_DIRS "webgpu.h")

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
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_device.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_instance.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_instance.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_pipeline_layout.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_pipeline_layout.h
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_queue.h
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

if(EXPORT_JIPU_WEBGPU)
  target_include_directories(jipu_webgpu
    PUBLIC
    $<INSTALL_INTERFACE:include>
  )
endif()

# link libraries
target_link_libraries(jipu_webgpu
  PUBLIC
  jipu::native
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

if(EXPORT_JIPU_WEBGPU)
  message(STATUS "jipu::webgpu will be installed")

  install(TARGETS jipu_webgpu
    EXPORT jipu_webgpu_targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  )

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/jipu/webgpu
    FILES_MATCHING PATTERN "*.h"
  )

  install(EXPORT jipu_webgpu_targets
    FILE jipu_webgpu-config.cmake
    NAMESPACE jipu::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/jipu_webgpu
  )
endif()
