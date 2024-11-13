cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE
	${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
    	CACHE FILEPATH "vcpkg toolchain file location")
project(spotifar)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../out")


add_subdirectory(spotify-http-relay)
