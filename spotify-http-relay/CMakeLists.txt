cmake_minimum_required(VERSION 3.5)

project(spotify-http-relay LANGUAGES CXX)


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
