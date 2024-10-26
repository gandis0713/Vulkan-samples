cmake_minimum_required(VERSION 3.22)

add_subdirectory(common)
add_subdirectory(native)
add_subdirectory(webgpu)

set(JIPU_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/jipu_proc.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/webgpu_proc.cpp
)

add_library(jipu_jipu
  SHARED
  ${JIPU_SRC_FILES}
)
add_library(jipu::jipu ALIAS jipu_jipu)

# link libraries
target_link_libraries(jipu_jipu
  PRIVATE
  jipu::webgpu
)

install(TARGETS jipu_jipu
  EXPORT jipu_jipu_targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(EXPORT jipu_jipu_targets
  FILE jipu_jipu-config.cmake
  NAMESPACE jipu::
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/jipu_jipu
)
