message(WARNING "**********Copying files to ${CURRENT_PACKAGES_DIR}")
message(WARNING "*********VCPKG_TARGET_IS_WINDOWS: ${VCPKG_TARGET_IS_WINDOWS}")
message(WARNING "***********VCPKG_TARGET_IS_MINGW: ${VCPKG_TARGET_IS_MINGW}")
message(WARNING "***********WITH_OPENEXR: ${WITH_OPENEXR}")

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/benchmark
    REF "v${VERSION}"
    SHA512 4e12114251c79a426873cfba6e27270b69fc980cef9a68e9cb3170f8e2e203f77dee19ab1e65cad51cd67e60991d3bbfdd52553f22522ce5e6c611b5aa07602c
    HEAD_REF main
    PATCHES
        fix_qnx.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        -DBENCHMARK_ENABLE_TESTING=OFF
        -DBENCHMARK_INSTALL_DOCS=OFF
        -Werror=old-style-cast
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/benchmark)

vcpkg_fixup_pkgconfig()


file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
