
# SOURCES
#-----------------------------------------------------------------------------------------
add_library(${PROJECT_NAME} SHARED
    spotifar.cpp
    plugin.cpp
    utils.cpp
    config.cpp
    hotkeys_handler.cpp
    librespot.cpp
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
    ui/dialogs/waiting.cpp
    ui/dialogs/settings/hotkeys.cpp
    ui/dialogs/settings/general.cpp
    ui/dialogs/settings/backend.cpp
    ui/dialogs/settings/caches.cpp
    ui/dialogs/filters/albums.cpp
    ../thirdparty/ObserverManager/ObserverManager/ObserverManager.cpp
    ../thirdparty/ObserverManager/ObserverManager/BaseObserverProtocol.cpp
    spotifar.vc.def
)

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

# DEPENDENCIES
#-----------------------------------------------------------------------------------------
find_package(httplib CONFIG REQUIRED)

find_package(RapidJSON CONFIG REQUIRED)

set(SPDLOG_WCHAR_FILENAMES ON)
find_package(spdlog CONFIG REQUIRED)

find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        httplib::httplib
        spdlog::spdlog_header_only
        rapidjson
)

# the library is not available on MinGW platform
if(NOT MINGW)
    find_package(unofficial-wintoast CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        unofficial::wintoast::wintoast
    )
endif()

# POST BUILD
#-----------------------------------------------------------------------------------------
set(OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
set(CARGO_BUILD_TYPE x$<CONFIG>)

message(STATUS "Packaging Spotifar docs")
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E
    copy_directory ${PROJECT_SOURCE_DIR}/docs ${OUTPUT_DIRECTORY}
)

find_program(CARGO_EXECUTABLE cargo)
if (CARGO_EXECUTABLE)

    set(LIBRESPOT_TARGET ${PROJECT_SOURCE_DIR}/thirdparty/librespot/target/${CARGO_BUILD_TYPE}/librespot.exe)

    if(NOT EXISTS "${LIBRESPOT_TARGET}")
        message(STATUS "Cargo executable is found, building Librespot")
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CARGO_EXECUTABLE} build --profile=${CARGO_BUILD_TYPE} --manifest-path=${PROJECT_SOURCE_DIR}\\thirdparty\\librespot\\Cargo.toml
            VERBATIM
            USES_TERMINAL
        )
    endif()

    message(STATUS "Copying Librespot executable to the output directory")
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        copy ${LIBRESPOT_TARGET} ${OUTPUT_DIRECTORY}
    )

else()
    message(WARNING "No cargo command is found, Libresepot building is skipped")
endif()