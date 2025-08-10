
# Fetch GoogleTest at configure time (pin a specific release)
include(FetchContent)
# If building with MSVC, align CRT usage to avoid MD/MT mismatches
# Note: this needs to be in the cache and set before bringing googletest in
set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT on Windows" FORCE)

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

# copugin Spotifar libraries to the tests folder
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