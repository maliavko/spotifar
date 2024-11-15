add_subdirectory(spotify_http_relay)


find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(${PROJECT_NAME} SHARED
	spotifar.cpp
	spotify.cpp
	utils.cpp
	browser.cpp
	config.cpp
	data_provider.cpp
	spotifar.vc.def
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${PROJECT_SOURCE_DIR}/thirdparty/farmanager/common
		${PROJECT_SOURCE_DIR}/thirdparty/farmanager/common/unicode)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		httplib::httplib
		nlohmann_json::nlohmann_json
		OpenSSL::SSL
		OpenSSL::Crypto)
