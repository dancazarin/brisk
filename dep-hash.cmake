# This script computes the hash of files that control dependency building, enabling version management and automatic
# rebuilding of dependencies.

# GLOB_RECURSE returns sorted list of files
file(GLOB_RECURSE FILES "${CMAKE_CURRENT_LIST_DIR}/cmake/ports/*" "${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/*"
     "${CMAKE_CURRENT_LIST_DIR}/cmake/triplets/*" "${CMAKE_CURRENT_LIST_DIR}/vcpkg.json")

set(HASHES)

foreach (FILE IN LISTS FILES)
    file(READ ${FILE} CONTENT)
    string(SHA256 HASH "${CONTENT}")
    message("${FILE} -> ${HASH}")
    list(APPEND HASHES "${HASH}")
endforeach ()

string(SHA256 GLOB_HASH "${HASHES}")
message(STATUS ${GLOB_HASH})
