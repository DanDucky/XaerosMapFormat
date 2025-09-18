CPMAddPackage("gh:catchorg/Catch2@3.4.0")

enable_testing()

add_executable(XaeroTests
        ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

target_link_libraries(XaeroTests PRIVATE Catch2::Catch2WithMain XaerosMapFormat)

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

catch_discover_tests(XaeroTests)
