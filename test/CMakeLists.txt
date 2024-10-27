cmake_minimum_required(VERSION 3.22)

add_subdirectory(base)

function(configure_test name)
  set(target ${name}_test)
  set(srcs
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}_test_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}_test.h
  )

  add_executable(${target} ${srcs})
  add_test(NAME ${target} COMMAND ${target})

  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_link_libraries(${target}
    PUBLIC
    jipu::test_base
  )
endfunction()

configure_test(copy)
configure_test(submit)
configure_test(buffer)
configure_test(texture)
configure_test(device)