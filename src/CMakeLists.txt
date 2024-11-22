
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(${PROJECT_NAME} SHARED
	spotifar.cpp
	utils.cpp
	browser.cpp
	config.cpp
	spotify/items.cpp
	spotify/controller.cpp
	spotifar.vc.def
)

target_precompile_headers(${PROJECT_NAME} PUBLIC stdafx.h)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		${PROJECT_BINARY_DIR}  # required for being able to include CMake generated version file
		${PROJECT_SOURCE_DIR}/src
		${PROJECT_SOURCE_DIR}/thirdparty/FarManager/plugins/common
		${PROJECT_SOURCE_DIR}/thirdparty/FarManager/plugins/common/unicode)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		httplib::httplib
		nlohmann_json::nlohmann_json
		OpenSSL::SSL
		OpenSSL::Crypto)

# copy docs to the final package
add_custom_command(
	TARGET ${PROJECT_NAME}
	POST_BUILD COMMAND ${CMAKE_COMMAND} -E
	copy_directory ${PROJECT_SOURCE_DIR}/docs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
)