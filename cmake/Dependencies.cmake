file (DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/external/cmake/get_cpm.cmake
)

include(${CMAKE_CURRENT_SOURCE_DIR}/external/cmake/get_cpm.cmake)

CPMAddPackage(
        NAME minizip-ng
        GIT_TAG 4.0.10
        GITHUB_REPOSITORY zlib-ng/minizip-ng
        OPTIONS
        "MZ_ZLIB ON"
        "MZ_BZIP2 OFF"
        "MZ_LZMA OFF"
        "MZ_ZSTD OFF"
        "MZ_OPENSSL OFF"
        "MZ_LIBBSD OFF"
        "MZ_ICONV OFF"
)

CPMAddPackage(
        NAME ztd.text
        GIT_TAG main
        GIT_SHALLOW ON
        GITHUB_REPOSITORY soasis/text
)

CPMAddPackage(
        NAME libnbt++
        GIT_TAG master
        GITHUB_REPOSITORY PrismLauncher/libnbtplusplus
        OPTIONS
        "NBT_USE_ZLIB OFF"
        "NBT_BUILD_TESTS OFF"
)
