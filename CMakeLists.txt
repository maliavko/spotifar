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

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # for some reason I needed to specify Clang keys for generating PDB symbols
    add_compile_options(-g -gcodeview)
    add_link_options(-fuse-ld=lld -g -Wl,--pdb=)
endif()

add_subdirectory(src)