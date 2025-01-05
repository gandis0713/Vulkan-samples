cmake_minimum_required(VERSION 3.22)

project(
    jipu_sample_base
    VERSION 0.1
    DESCRIPTION "jipu sample base"
    LANGUAGES CXX
)

find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE
    NAMES "tiny_gltf.h"
    HINTS ${VCPKG_INSTALLED_DIR}
    REQUIRED
)

if(NOT ANDROID)
    find_package(SDL2 CONFIG REQUIRED)
endif()

list(APPEND SRC_FILES
    camera.cpp
    camera.h
    sample.cpp
    sample.h
    wgpu_sample.cpp
    wgpu_sample.h
    wgpu_im_gui.cpp
    wgpu_im_gui.h
    fps.cpp
    fps.h
    window.cpp
    window.h
    vertex.cpp
    vertex.h
    im_gui.cpp
    im_gui.h
    model.cpp
    model.h
    file.cpp
    file.h
    light.cpp
    light.h
    khronos_texture.cpp
    khronos_texture.h
    image.cpp
    image.h
    hpc_watcher.cpp
    hpc_watcher.h
    webgpu_api.cpp
    webgpu_api.h
)

if(ANDROID)
    list(APPEND SRC_FILES
        window_android.cpp
    )
else()
    list(APPEND SRC_FILES
        window_sdl.cpp
    )

    if(APPLE)
        list(APPEND SRC_FILES
            window_cocoa.mm
        )
    elseif(MSVC)
        list(APPEND SRC_FILES
            window_win32.cpp
        )
    endif()
endif()

add_subdirectory(hpc)
add_library(jipu_sample_base STATIC ${SRC_FILES})
add_library(jipu::sample_base ALIAS jipu_sample_base)

target_include_directories(jipu_sample_base
    PRIVATE
    ${Stb_INCLUDE_DIR}
    ${TINYGLTF_INCLUDE}
)

target_link_libraries(jipu_sample_base
    PUBLIC
    glm::glm
    jipu::common
    jipu::native
    jipu::jipu
    PRIVATE
    tinyobjloader::tinyobjloader
    KTX::ktx
    hpc::hpc
    imgui::imgui
)

if(USE_DAWN_WEBGPU)
    target_compile_definitions(jipu_sample_base
        PUBLIC
        "USE_DAWN_HEADER"
    )
endif()

if(NOT ANDROID)
    target_link_libraries(jipu_sample_base
        PUBLIC
        SDL2::SDL2 SDL2::SDL2main
    )
else()
    find_package(game-activity REQUIRED CONFIG)
    find_library(log-lib log)
    target_link_libraries(jipu_sample_base
        PUBLIC
        game-activity::game-activity # game-activity::game-activity_static
        android
        ${log-lib}
    )
endif()

target_include_directories(jipu_sample_base
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hpc/hpc/include>
)