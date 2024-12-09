
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

set(SPDLOG_WCHAR_FILENAMES ON)
find_package(spdlog CONFIG REQUIRED)


add_library(${PROJECT_NAME} SHARED
	spotifar.cpp
	plugin.cpp
	utils.cpp
	config.cpp
	spotify/items.cpp
	spotify/api.cpp
	ui/panel.cpp
	ui/views.cpp
	ui/player_dialog.cpp
	ui/config_dialog.cpp
	../thirdparty/ObserverManager/ObserverManager/ObserverManager.cpp
	../thirdparty/ObserverManager/ObserverManager/BaseObserverProtocol.cpp
	spotifar.vc.def
)

# the epxperimenting way to pass variable to preprocessor
#target_compile_definitions(${PROJECT_NAME} PRIVATE MY_VAR="${PROJECT_NAME}")

target_precompile_headers(${PROJECT_NAME} PUBLIC stdafx.h)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${PROJECT_BINARY_DIR}  # required for being able to include CMake generated version file
		${PROJECT_SOURCE_DIR}/src
		${PROJECT_SOURCE_DIR}/thirdparty/ObserverManager/ObserverManager
		${PROJECT_SOURCE_DIR}/thirdparty/FarManager/plugins/common
		${PROJECT_SOURCE_DIR}/thirdparty/FarManager/plugins/common/unicode)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		httplib::httplib
		nlohmann_json::nlohmann_json
		OpenSSL::SSL
		OpenSSL::Crypto
		spdlog::spdlog_header_only)

# copy docs to the final package
add_custom_command(
	TARGET ${PROJECT_NAME}
	POST_BUILD COMMAND ${CMAKE_COMMAND} -E
	copy_directory ${PROJECT_SOURCE_DIR}/docs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
)