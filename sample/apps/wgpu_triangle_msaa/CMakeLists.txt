configure_sample(wgpu_triangle_msaa)

if(ANDROID)
    set(JINLIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../android/apps/wgpu_triangle_msaa/src/main/jniLibs/arm64-v8a)

    find_library(DAWN_LIB REQUIRED
        NAMES webgpu_dawn
        PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib
    )

    add_custom_command(
        TARGET wgpu_triangle_msaa
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${JINLIBS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${DAWN_LIB}" "${JINLIBS_DIR}"
        COMMENT "Copying webgpu_dawn library to ${JINLIBS_DIR}"
    )
endif()