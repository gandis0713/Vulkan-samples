cmake_minimum_required(VERSION 3.22)

project(
    hpc
    VERSION 0.1
    DESCRIPTION "hardware performance counter"
    LANGUAGES CXX
)

if(ANDROID)
    add_subdirectory(backend)
endif()

add_subdirectory(hpc)