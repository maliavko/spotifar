include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT on Windows" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable googletest install" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(spotifar_tests
    utils.cpp)

target_link_libraries(spotifar_tests
    PRIVATE
        spotifar
        gtest
        gtest_main
)

include(GoogleTest)

# copying Spotifar libraries to the tests folder
add_custom_command(
    TARGET spotifar_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E
    copy_directory $<TARGET_FILE_DIR:spotifar> $<TARGET_FILE_DIR:spotifar_tests>
    COMMAND_EXPAND_LISTS
    VERBATIM)

set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE POST_BUILD)
set(CTEST_OUTPUT_ON_FAILURE ON)

gtest_discover_tests(spotifar_tests
  # Optional: add a prefix to test names in CTest
  # TEST_PREFIX "unit:"
)