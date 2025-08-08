cmake_minimum_required(VERSION 3.5...3.10)

# enum VERSION_STAGE
# {
# VS_RELEASE      = 0,
#	VS_ALPHA        = 1,
#	VS_BETA         = 2,
#	VS_RC           = 3,
#	VS_SPECIAL      = 4,
#	VS_PRIVATE      = 5,
#};
#
# Major - plugin build number
# Minor - version stage id from above
project(spotifar VERSION 1.1 LANGUAGES CXX)

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
target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  UNICODE
  VC_EXTRALEAN
  WIN32_LEAN_AND_MEAN
  _WINSOCKAPI_
)