vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO guillaumeblanc/ozz-animation
    REF "${VERSION}"
    SHA512 e815e7ca56393a065a833f990dbfd20fa57f8c8f6502b8ea95934b2d1e6ada1e29e048f113cd4db780cd350036424364ee15176a8c788e7bc8c9e35413bdf766
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -Dozz_build_samples=OFF
        -Dozz_build_howtos=OFF
        -Dozz_build_tests=OFF
        -Dozz_build_gltf=OFF
        -Dozz_build_fbx=OFF
        -Dozz_build_data=OFF
        -Dozz_build_tools=OFF
        -DCMAKE_DEBUG_POSTFIX=_d
        -DCMAKE_RELEASE_POSTFIX=_r
)

vcpkg_cmake_install()

# Install custom config file
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/ozz-animationConfig.cmake.in"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    RENAME "ozz-animationConfig.cmake")

# Remove debug include dir
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Install license
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
