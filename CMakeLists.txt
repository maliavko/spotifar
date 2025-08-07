cmake_minimum_required(VERSION 3.5...3.10)

# enum VERSION_STAGE
# {
# 	VS_RELEASE      = 0,
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

add_subdirectory(src)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "spotifar"
    PREFIX ""
)

# Add debug config compile definition for sources
target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  UNICODE
  VC_EXTRALEAN
  WIN32_LEAN_AND_MEAN
  _WINSOCKAPI_
  RAPIDJSON_HAS_STDSTRING=1
  FMT_HEADER_ONLY
  SPDLOG_WCHAR_FILENAMES
  SPDLOG_USE_STD_FORMAT=0
)