cmake_minimum_required(VERSION 3.22)

add_subdirectory(common)
add_subdirectory(native)
add_subdirectory(webgpu)

set(JIPU_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/native_proc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proc.cpp
)

add_library(jipu
  SHARED
  ${JIPU_SRC_FILES}
)
add_library(jipu::jipu ALIAS jipu)

# link libraries
target_link_libraries(jipu
  PRIVATE
  jipu::webgpu
)

install(TARGETS jipu
  EXPORT jipu_targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(EXPORT jipu_targets
  FILE jipu-config.cmake
  NAMESPACE jipu::
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/jipu
)
