project(spotify-http-relay LANGUAGES CXX)


find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)


add_executable(${PROJECT_NAME}
	entry.cpp
	utils.cpp
	spotify_http_relay.cpp
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		httplib::httplib
		nlohmann_json::nlohmann_json
		OpenSSL::SSL
		OpenSSL::Crypto)

target_precompile_headers(${PROJECT_NAME} PUBLIC stdafx.h)
