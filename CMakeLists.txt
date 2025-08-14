cmake_minimum_required(VERSION 3.5...3.10)

project(spotifar VERSION 0.9.0 LANGUAGES CXX)

set(PLUGIN_NAME "${PROJECT_NAME}")
set(PLUGIN_FILENAME "${PROJECT_NAME}.dll")
set(PLUGIN_DESC "A front-end client for listening to music from Spotify")
set(PLUGIN_AUTHOR "IgorM")

# enum VERSION_STAGE
# {
#   VS_RELEASE      = 0,
#	  VS_ALPHA        = 1,
#	  VS_BETA         = 2,
#	  VS_RC           = 3,
#	  VS_SPECIAL      = 4,
#	  VS_PRIVATE      = 5,
#};
set(PLUGIN_VER_STAGE VS_RELEASE)

set(FAR_VER_MAJOR 3)
set(FAR_VER_MINOR 0)
set(FAR_VER_REV 0)
set(FAR_VER_BUILD 0)
set(FAR_VER_STAGE VS_RELEASE)

configure_file(src/version.hpp.in version.hpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# GitHub agents use GCC 12, which generates this obsolete warning
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-stringop-overread")
endif()

add_subdirectory(src)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "${PROJECT_NAME}"
    PREFIX ""
)

# Add debug config compile definition for sources
target_compile_definitions(${PROJECT_NAME} PUBLIC
  $<$<CONFIG:Debug>:DEBUG>
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  UNICODE
  VC_EXTRALEAN
)

if(TESTING)
  enable_testing()

  # Add tests directory (contains a test executable and discovery)
  add_subdirectory(tests)

  target_compile_definitions(${PROJECT_NAME} PRIVATE TESTING=1)
endif()