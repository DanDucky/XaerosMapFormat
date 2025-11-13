CPMAddPackage(
        NAME args
        GIT_TAG 6.4.7
        GITHUB_REPOSITORY Taywee/args
)

CPMAddPackage(
        NAME stb
        GIT_TAG master
        GITHUB_REPOSITORY nothings/stb
        DOWNLOAD_ONLY ON
)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

if (XAERO_GENERATE_RESOURCES)
    message(STATUS "Configuring XaeroRenderer")
    add_executable(XaeroRenderer ${CMAKE_CURRENT_SOURCE_DIR}/XaeroRenderer.cpp)
    target_link_libraries(XaeroRenderer PRIVATE taywee::args stb XaerosMapFormat)
endif ()

message(STATUS "Configuring XaeroReprocess")
add_executable(XaeroReprocess ${CMAKE_CURRENT_SOURCE_DIR}/XaeroReprocess.cpp)
target_link_libraries(XaeroReprocess PRIVATE taywee::args XaerosMapFormat)
