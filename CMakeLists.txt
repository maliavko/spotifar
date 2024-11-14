cmake_minimum_required(VERSION 3.5)

project(spotifar)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_subdirectory(spotify-http-relay)


find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(${PROJECT_NAME} SHARED
	src/spotifar.cpp
	src/spotify.cpp
	src/utils.cpp
	src/browser.cpp
	src/config.cpp
	src/data_provider.cpp
	src/spotifar.vc.def
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/thirdparty/farmanager/common
		${PROJECT_SOURCE_DIR}/thirdparty/farmanager/common/unicode)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		httplib::httplib
		nlohmann_json::nlohmann_json
		OpenSSL::SSL OpenSSL::Crypto)
