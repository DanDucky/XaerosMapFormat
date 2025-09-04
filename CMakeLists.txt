cmake_minimum_required(VERSION 3.31.0)
set(CMAKE_CXX_STANDARD 23)
project(XaerosMapFormat VERSION 0.1 DESCRIPTION "library for parsing XaerosMap data and rendering it to images")

option(XAERO_BUILD_TESTS "builds catch2 tests" ON)
option(XAERO_GENERATE_RESOURCES "generates minecraft lookup tables for colors and names, requires java and python" ON)
option(XAERO_BUILD_EXAMPLES "builds example programs" ON)
set(XAERO_MINECRAFT_VERSION "latest" CACHE STRING "override for Minecraft version to fetch resources from")

add_library(XaerosMapFormat
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Map.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/RegionTools.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util/ByteInputStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util/ByteOutputStream.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util/StringUtils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/types/RegionImage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/types/Region.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/types/LookupTypes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/types/BlockState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lookups/LegacyCompatibility.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lookups/LegacyCompatibility.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lookups/BlockLookups.cpp
)

target_compile_definitions(XaerosMapFormat PUBLIC XAERO_REGION_VERSION_MAJOR=7)
target_compile_definitions(XaerosMapFormat PUBLIC XAERO_REGION_VERSION_MINOR=8)

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

set(WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/XaerosMapFormat")

set(STATE_ID_LOOKUP "${WORKING_DIRECTORY}/legacy/src/lookups/StateIDLookup.cpp")

if (NOT EXISTS ${STATE_ID_LOOKUP})
    message(STATUS "Generating legacy lookups...")

    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ByteInputUtils.cmake)

    set(MOD_JAR "${WORKING_DIRECTORY}/mod.jar")
    set(MOD_EXTRACTED "${WORKING_DIRECTORY}/mod_extracted")

    if (NOT EXISTS ${MOD_JAR})
        file (DOWNLOAD
                "https://cdn.modrinth.com/data/NcUtCpym/versions/3FwkAdEs/XaerosWorldMap_1.29.17_Forge_1.14.4.jar"
                "${MOD_JAR}"
                STATUS MOD_STATUS
        )

        if (NOT MOD_STATUS EQUAL 0)
            file(REMOVE "${MOD_JAR}")
            message(FATAL_ERROR "Failed to download mod from https://cdn.modrinth.com/data/1bokaNcj/versions/StqWcPqA/Xaeros_Minimap_25.2.12_Fabric_1.21.8.jar")
        endif ()
    endif ()

    if (NOT EXISTS ${MOD_EXTRACTED})
        file(ARCHIVE_EXTRACT
                INPUT ${MOD_JAR}
                DESTINATION ${MOD_EXTRACTED}
        )
    endif()

    file(READ "${MOD_EXTRACTED}/assets/xaeroworldmap/vanilla_states.dat" VANILLA_STATES HEX)

    set(POSITION 0)
    set(STATE_INFO "")
    set(META_INFO "")
    set(LAST_ID -1)
    string(LENGTH "${VANILLA_STATES}" VANILLA_LENGTH)
    math(EXPR VANILLA_LENGTH "${VANILLA_LENGTH} / 2")

    while (NOT ${POSITION} GREATER_EQUAL ${VANILLA_LENGTH})
        read_uint("${VANILLA_STATES}" ${POSITION} 4 ID POSITION)

        math(EXPR BLOCK_ID "${ID} & 4095")
        math(EXPR META_ID "${ID} >> 12 & 1048575")

        math(EXPR SKIPPED "${BLOCK_ID} - (${LAST_ID} + 1)")

        if ((NOT "${META_INFO}" STREQUAL "") AND NOT ${LAST_ID} EQUAL ${BLOCK_ID}) # push last meta_info into state_info
            list(JOIN META_INFO ",\n" META_INFO_JOINED)
            set(META_INFO "")
            list(APPEND STATE_INFO "{${META_INFO_JOINED}}")

            if (NOT ${SKIPPED} LESS_EQUAL 0)
                string(REPEAT "{};" ${SKIPPED} PADDING)
                string(REGEX REPLACE ".$" "" PADDING "${PADDING}")
                list(APPEND STATE_INFO "${PADDING}")
            endif ()
        endif ()

        set(LAST_ID ${BLOCK_ID})

        math(EXPR POSITION "${POSITION} + 3") # skip over initial tag

        math(EXPR POSITION "${POSITION} + 1") # skip over tag type

        read_uint("${VANILLA_STATES}" ${POSITION} 2 TAG_SIZE POSITION)

        read_string("${VANILLA_STATES}" ${POSITION} ${TAG_SIZE} TAG_NAME POSITION)

        set(PROPERTIES "")

        if ("${TAG_NAME}" STREQUAL "Properties")
            while(1 EQUAL 1)
                read_uint("${VANILLA_STATES}" ${POSITION} 1 TAG_TYPE POSITION)
                if (TAG_TYPE EQUAL 0) # end of properties
                    break()
                endif ()

                read_uint("${VANILLA_STATES}" ${POSITION} 2 TAG_SIZE POSITION)

                read_string("${VANILLA_STATES}" ${POSITION} ${TAG_SIZE} TAG_NAME POSITION)

                read_uint("${VANILLA_STATES}" ${POSITION} 2 VALUE_SIZE POSITION)

                read_string("${VANILLA_STATES}" ${POSITION} ${VALUE_SIZE} VALUE POSITION)

                list(APPEND PROPERTIES "{\"${TAG_NAME}\",\"${VALUE}\"}")
            endwhile ()

            math(EXPR POSITION "${POSITION} + 1 + 2 + 4") # skip over known info

        endif ()

        read_uint("${VANILLA_STATES}" ${POSITION} 2 NAME_SIZE POSITION)

        read_string("${VANILLA_STATES}" ${POSITION} ${NAME_SIZE} BLOCK_NAME POSITION)

        string(REPLACE "minecraft:" "" BLOCK_NAME "${BLOCK_NAME}")

        math(EXPR POSITION "${POSITION} + 1") # skip over known end bytes

        string(LENGTH "${PROPERTIES}" PROPERTIES_LENGTH)
        if (${PROPERTIES_LENGTH} GREATER 0)
            list(JOIN PROPERTIES "," PROPERTIES)
        endif ()

        list(APPEND META_INFO "BlockState{\"${BLOCK_NAME}\",nbt::tag_compound{${PROPERTIES}}}")

    endwhile ()

    if (NOT "${META_INFO}" STREQUAL "") # push last meta_info into state_info
        list(JOIN META_INFO ",\n" META_INFO_JOINED)
        list(APPEND STATE_INFO "{${META_INFO_JOINED}}")
    endif ()

    list(JOIN STATE_INFO ",\n" STATE_INFO)

    set(STATE_ID_CONTENT
    "#include \"lookups/LegacyCompatibility.hpp\"
    namespace xaero {

    const StateIDLookup stateIDLookup = {${STATE_INFO}}\;const std::size_t stateIDLookupSize = ${BLOCK_ID} + 1\;}")

    file(WRITE "${STATE_ID_LOOKUP}" ${STATE_ID_CONTENT})

    message(STATUS "Generated legacy lookups")

endif ()

add_library(XaerosLegacy OBJECT ${STATE_ID_LOOKUP})
target_link_libraries(XaerosLegacy PRIVATE nbt++)
target_include_directories(XaerosLegacy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(XaerosLegacy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(XaerosMapFormat PRIVATE XaerosLegacy) # legacy lookups

if (XAERO_GENERATE_RESOURCES)

    find_package(Java 21 REQUIRED COMPONENTS Runtime)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # create .venv

    set(VENV_DIR "${WORKING_DIRECTORY}/.venv")

    if (WIN32) # fuck windows
        set(VENV_PYTHON "${VENV_DIR}/Scripts/python.exe")
    else ()
        set(VENV_PYTHON "${VENV_DIR}/bin/python3")
    endif ()

    if(NOT EXISTS ${VENV_PYTHON})
        execute_process(
                COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
                RESULT_VARIABLE VENV_RESULT
        )
        if(NOT VENV_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to create Python virtual environment")
        endif()
    endif()

    # install pillow

    execute_process(
            COMMAND ${VENV_PYTHON} -m pip install Pillow argparse numpy
            RESULT_VARIABLE PIP_RESULT
    )
    if(NOT PIP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to install python libraries in virtual environment")
    endif()

    # pip installed, python and java found

    # paraphrased from https://github.com/adepierre/Botcraft/blob/a673577228dbab69f63ea8f3298979aca0be8c10/cmake/mc_urls.cmake#L63

    set(VERSION_MANIFEST "${WORKING_DIRECTORY}/version_manifest_v2.json")
    set(EXPECTED_VERSION_PACKAGE "${WORKING_DIRECTORY}/${XAERO_MINECRAFT_VERSION}.json")

    # will always be true for "latest" because we don't make latest.json
    if (NOT EXISTS ${EXPECTED_VERSION_PACKAGE})
        file(DOWNLOAD
                "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"
                ${VERSION_MANIFEST}
                STATUS MANIFEST_RESULT
        )

        if (NOT MANIFEST_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to download mojang's version manifest!")
        endif ()

        file(READ ${VERSION_MANIFEST} VERSION_MANIFEST_CONTENT)

        string(JSON VERSIONS_ARRAY_LENGTH LENGTH ${VERSION_MANIFEST_CONTENT} "versions")
        math(EXPR VERSIONS_ARRAY_LENGTH "${VERSIONS_ARRAY_LENGTH}-1")

        foreach (I RANGE ${VERSIONS_ARRAY_LENGTH})
            string(JSON VERSION_TYPE GET ${VERSION_MANIFEST_CONTENT} "versions" ${I} "type")
            string(JSON LATEST_VERSION GET ${VERSION_MANIFEST_CONTENT} "versions" ${I} "id")
            if ((${VERSION_TYPE} STREQUAL "release" AND ${XAERO_MINECRAFT_VERSION} STREQUAL "latest") OR ${LATEST_VERSION} STREQUAL "${XAERO_MINECRAFT_VERSION}")
                string(JSON MINECRAFT_VERSION_URL GET ${VERSION_MANIFEST_CONTENT} "versions" ${I} "url")
                break()
            endif ()
            set(LATEST_VERSION "NOT FOUND")
        endforeach ()

        if (${LATEST_VERSION} STREQUAL "NOT FOUND")
            message(FATAL_ERROR "Could not find a Minecraft version for ${XAERO_MINECRAFT_VERSION}")
        endif ()

        message(STATUS "Selected ${LATEST_VERSION} as the Minecraft release")
    else ()
        set(LATEST_VERSION "${XAERO_MINECRAFT_VERSION}")
    endif ()

    set(MINECRAFT_VERSION_PACKAGE "${WORKING_DIRECTORY}/${LATEST_VERSION}.json")

    if (NOT EXISTS ${MINECRAFT_VERSION_PACKAGE})
        set(REQUIRES_DOWNLOAD ON)

        file(DOWNLOAD
                ${MINECRAFT_VERSION_URL}
                ${MINECRAFT_VERSION_PACKAGE}
                STATUS VERSION_RESULT
        )

        if (NOT VERSION_RESULT EQUAL 0)
            file(REMOVE ${MINECRAFT_VERSION_PACKAGE})
            message(FATAL_ERROR "Failed to download the Minecraft version package")
        endif ()
    endif()

    file (READ ${MINECRAFT_VERSION_PACKAGE} VERSION_PACKAGE_CONTENT)

    string(JSON SERVER_URL GET ${VERSION_PACKAGE_CONTENT} "downloads" "server" "url")
    string(JSON CLIENT_URL GET ${VERSION_PACKAGE_CONTENT} "downloads" "client" "url")

    set(SERVER_JAR "${WORKING_DIRECTORY}/server.jar")
    set(CLIENT_JAR "${WORKING_DIRECTORY}/client.jar")

    if ((NOT EXISTS ${SERVER_JAR}) OR REQUIRES_DOWNLOAD)
        file (DOWNLOAD
                ${SERVER_URL}
                ${SERVER_JAR}
                STATUS SERVER_STATUS
        )

        if (NOT SERVER_STATUS EQUAL 0)
            file(REMOVE ${SERVER_JAR})
            message(FATAL_ERROR "Failed to fetch server jar")
        endif ()
    endif ()

    if ((NOT EXISTS ${CLIENT_JAR}) OR REQUIRES_DOWNLOAD)
        file (DOWNLOAD
                ${CLIENT_URL}
                ${CLIENT_JAR}
                STATUS CLIENT_STATUS
        )

        if (NOT CLIENT_STATUS EQUAL 0)
            file(REMOVE ${CLIENT_JAR})
            message(FATAL_ERROR "Failed to fetch client jar")
        endif ()
    endif ()

    if ((NOT EXISTS ${WORKING_DIRECTORY}/generated/reports) OR REQUIRES_DOWNLOAD)
        execute_process(
                COMMAND ${Java_JAVA_EXECUTABLE}
                -DbundlerMainClass=net.minecraft.data.Main
                -jar ${SERVER_JAR}
                --reports
                WORKING_DIRECTORY ${WORKING_DIRECTORY}
                RESULT_VARIABLE DATA_GENERATOR_RESULT
        )

        if (NOT DATA_GENERATOR_RESULT EQUAL 0)
            file(REMOVE_RECURSE ${WORKING_DIRECTORY}/generated/reports)
            message(FATAL_ERROR "Could not run Minecraft data generator!")
        endif ()
    endif()

    set(CLIENT_JAR_EXTRACTED "${WORKING_DIRECTORY}/client_extracted")

    if ((NOT EXISTS ${CLIENT_JAR_EXTRACTED}) OR REQUIRES_DOWNLOAD)
        file(ARCHIVE_EXTRACT
                INPUT ${CLIENT_JAR}
                DESTINATION ${CLIENT_JAR_EXTRACTED}
        )
    endif()

    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/generation/generator.py
            ${WORKING_DIRECTORY}/generator.py.stamp
            COPYONLY
    )

    file(SHA256 ${CMAKE_CURRENT_SOURCE_DIR}/generation/generator.py CURRENT_HASH)
    set(REQUIRES_GENERATION OFF)

    if (NOT EXISTS ${WORKING_DIRECTORY}/GeneratorHash.txt)
        file(WRITE ${WORKING_DIRECTORY}/GeneratorHash.txt ${CURRENT_HASH})
        set(REQUIRES_GENERATION ON)
    else ()
        file(READ ${WORKING_DIRECTORY}/GeneratorHash.txt PREVIOUS_HASH)
        string(STRIP "${PREVIOUS_HASH}" PREVIOUS_HASH)
        if (NOT PREVIOUS_HASH STREQUAL CURRENT_HASH)
            set(REQUIRES_GENERATION ON)
            file(WRITE ${WORKING_DIRECTORY}/GeneratorHash.txt ${CURRENT_HASH})
        endif ()
    endif ()

    set(GENERATED_FILE_NAMES "BlockLookups")

    if ((NOT EXISTS ${WORKING_DIRECTORY}/generated/manifest.txt) OR REQUIRES_DOWNLOAD OR REQUIRES_GENERATION)
        message(STATUS "Running generation script")

        execute_process(
                COMMAND
                ${VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/generation/generator.py
                --client ${CLIENT_JAR_EXTRACTED}
                --reports ${WORKING_DIRECTORY}/generated/reports
                --output_dir ${WORKING_DIRECTORY}/generated
                --file_names ${GENERATED_FILE_NAMES}
                RESULT_VARIABLE GENERATOR_RESULT
        )

        if (NOT GENERATOR_RESULT EQUAL 0)
            file(REMOVE ${WORKING_DIRECTORY}/generated/manifest.txt)
            message(FATAL_ERROR "Failed to generate lookup headers!")
        endif ()
    endif()

    file(STRINGS ${WORKING_DIRECTORY}/generated/manifest.txt SOURCES_LIST)

    add_library(XaerosLookups OBJECT ${SOURCES_LIST})
    target_link_libraries(XaerosLookups PRIVATE nbt++)
    target_include_directories(XaerosLookups PRIVATE ${WORKING_DIRECTORY}/generated/include)
    target_compile_definitions(XaerosLookups PUBLIC XAERO_DEFAULT_LOOKUPS)
    target_include_directories(XaerosLookups PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(XaerosMapFormat PUBLIC XaerosLookups)
endif ()

target_link_libraries(XaerosMapFormat PUBLIC nbt++)
target_link_libraries(XaerosMapFormat PRIVATE ztd::text minizip)

set_target_properties(XaerosMapFormat PROPERTIES VERSION ${PROJECT_VERSION})

target_include_directories(XaerosMapFormat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(XaerosMapFormat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${minizip-ng_SOURCE_DIR})

if (XAERO_BUILD_TESTS)
    CPMAddPackage("gh:catchorg/Catch2@3.4.0")

    enable_testing()

    add_executable(tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp
    )

    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain XaerosMapFormat)

    include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

    catch_discover_tests(tests)
endif ()

if (XAERO_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif ()

install(TARGETS XaerosMapFormat
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
