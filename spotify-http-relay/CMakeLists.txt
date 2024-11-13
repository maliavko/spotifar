cmake_minimum_required(VERSION 3.5)

#set(CMAKE_TOOLCHAIN_FILE
#	${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
#    	CACHE FILEPATH "vcpkg toolchain file location")

project(spotify-http-relay LANGUAGES CXX)


#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/_output)
#set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)


find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
	src/entry.cpp
	src/utils.cpp
	src/spotify_http_relay.cpp
)

target_include_directories(${PROJECT_NAME}
	PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
	PRIVATE httplib::httplib nlohmann_json::nlohmann_json OpenSSL::SSL OpenSSL::Crypto)
