cmake_minimum_required(VERSION 3.31.0)
set(CMAKE_CXX_STANDARD 23)
project(XaerosMapFormat VERSION 0.1.1 DESCRIPTION "library for parsing XaerosMap data and rendering it to images")

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

add_library(Xaero::XaerosMapFormat ALIAS XaerosMapFormat)
add_library(XaerosMapFormat::XaerosMapFormat ALIAS XaerosMapFormat)

target_compile_definitions(XaerosMapFormat PUBLIC XAERO_REGION_VERSION_MAJOR=7)
target_compile_definitions(XaerosMapFormat PUBLIC XAERO_REGION_VERSION_MINOR=8)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake)

set(WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/XaerosMapFormat")

set(STATE_ID_LOOKUP "${WORKING_DIRECTORY}/legacy/src/lookups/StateIDLookup.cpp")

if (NOT EXISTS ${STATE_ID_LOOKUP})
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/StateIDLookup.cmake)
endif ()

add_library(XaerosLegacy OBJECT ${STATE_ID_LOOKUP})
target_link_libraries(XaerosLegacy PRIVATE nbt++)
target_include_directories(XaerosLegacy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(XaerosLegacy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(XaerosMapFormat PRIVATE XaerosLegacy) # legacy lookups

if (XAERO_GENERATE_RESOURCES)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ColorLookup.cmake)
endif ()

target_link_libraries(XaerosMapFormat PUBLIC nbt++)
target_link_libraries(XaerosMapFormat PRIVATE ztd::text minizip)

set_target_properties(XaerosMapFormat PROPERTIES VERSION ${PROJECT_VERSION})

target_include_directories(XaerosMapFormat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(XaerosMapFormat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${minizip-ng_SOURCE_DIR})

if (XAERO_BUILD_TESTS)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif ()

if (XAERO_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif ()

install(TARGETS XaerosMapFormat
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
