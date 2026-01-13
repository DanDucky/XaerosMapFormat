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
