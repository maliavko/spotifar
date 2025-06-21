
find_package(httplib CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(unofficial-wintoast CONFIG REQUIRED)

set(SPDLOG_WCHAR_FILENAMES ON)
find_package(spdlog CONFIG REQUIRED)

find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")


add_library(${PROJECT_NAME} SHARED
	spotifar.cpp
	plugin.cpp
	utils.cpp
	config.cpp
	playback_handler.cpp
	spotify/items.cpp
	spotify/auth.cpp
	spotify/history.cpp
	spotify/api.cpp
	spotify/playback.cpp
	spotify/devices.cpp
	spotify/cache.cpp
	spotify/requesters.cpp
	spotify/releases.cpp
	spotify/library.cpp
	ui/types.cpp
	ui/notifications.cpp
	ui/panel.cpp
	ui/player.cpp
	ui/events.cpp
	ui/controls.cpp
	ui/views/view.cpp
	ui/views/root.cpp
	ui/views/albums.cpp
	ui/views/artists.cpp
	ui/views/tracks.cpp
	ui/views/playlists.cpp
	ui/dialogs/menus.cpp
	ui/dialogs/dialog.cpp
	ui/dialogs/search.cpp
	ui/dialogs/search_results.cpp
	ui/dialogs/settings/hotkeys.cpp
	ui/dialogs/settings/general.cpp
	ui/dialogs/settings/backend.cpp
	ui/dialogs/settings/caches.cpp
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
		${PROJECT_SOURCE_DIR}/thirdparty/generator/include
		${PROJECT_SOURCE_DIR}/thirdparty/FarManager/plugins/common
		${PROJECT_SOURCE_DIR}/thirdparty/FarManager/plugins/common/unicode
		${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		httplib::httplib
		OpenSSL::SSL
		OpenSSL::Crypto
		spdlog::spdlog_header_only
		rapidjson
		unofficial::wintoast::wintoast
)

# copy docs to the final package
add_custom_command(
	TARGET ${PROJECT_NAME}
	POST_BUILD COMMAND ${CMAKE_COMMAND} -E
	copy_directory ${PROJECT_SOURCE_DIR}/docs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
)