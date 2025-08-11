set(GUID_HEADER "${CMAKE_BINARY_DIR}/guids.hpp")
file(WRITE "${GUID_HEADER}" "// Auto-generated GUIDs\n#pragma once\n\nnamespace spotifar { namespace guids {\n")

foreach(NAME IN LISTS GUID_NAMES)
    # Generate GUID string (format: 12345678-1234-5678-1234-56789abcdef0)
    string(UUID ${NAME}_STR NAMESPACE 00000000-0000-0000-0000-000000000000 NAME "${NAME}" TYPE SHA1)
    set(GUID_STR "${${NAME}_STR}")

    # Parse fields
    string(SUBSTRING "${GUID_STR}" 0 8 d1)
    string(SUBSTRING "${GUID_STR}" 9 4 d2)
    string(SUBSTRING "${GUID_STR}" 14 4 d3)
    string(SUBSTRING "${GUID_STR}" 19 2 d4_0)
    string(SUBSTRING "${GUID_STR}" 21 2 d4_1)
    string(SUBSTRING "${GUID_STR}" 24 2 d4_2)
    string(SUBSTRING "${GUID_STR}" 26 2 d4_3)
    string(SUBSTRING "${GUID_STR}" 28 2 d4_4)
    string(SUBSTRING "${GUID_STR}" 30 2 d4_5)
    string(SUBSTRING "${GUID_STR}" 32 2 d4_6)
    string(SUBSTRING "${GUID_STR}" 34 2 d4_7)

    # Write to header
    file(APPEND "${GUID_HEADER}" "\tinline constexpr GUID ${NAME} = { 0x${d1}, 0x${d2}, 0x${d3}, { 0x${d4_0}, 0x${d4_1}, 0x${d4_2}, 0x${d4_3}, 0x${d4_4}, 0x${d4_5}, 0x${d4_6}, 0x${d4_7} } };\n")
endforeach()

file(APPEND "${GUID_HEADER}" "} // namespace guids\n} // namespace spotifar\n")
include_directories(${CMAKE_BINARY_DIR})