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

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang keys for generating PDB symbols
    target_compile_options(${PROJECT_NAME} PRIVATE -g -gcodeview)
    #target_link_options(${PROJECT_NAME} PRIVATE -fuse-ld=lld -g -Wl,--pdb=$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.pdb)
    target_link_options(${PROJECT_NAME} PRIVATE -fuse-ld=lld -g -O0 -fPIC -Wl)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "spotifar"
    PREFIX ""
)