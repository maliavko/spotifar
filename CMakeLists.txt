cmake_minimum_required(VERSION 3.5)


# enum VERSION_STAGE
# {
# 	VS_RELEASE                      = 0,
#	VS_ALPHA                        = 1,
#	VS_BETA                         = 2,
#	VS_RC                           = 3,
#	VS_SPECIAL                      = 4,
#	VS_PRIVATE                      = 5,
#};
#
# Major - plugin build number
# Minor - version stage id from above
project(spotifar VERSION 1.1)

configure_file(src/version.hpp.in version.hpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(src)