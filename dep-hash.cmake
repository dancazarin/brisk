# This script computes the hash of files that control dependency building, enabling version management and automatic
# rebuilding of dependencies.

# GLOB_RECURSE returns sorted list of files
file(GLOB_RECURSE FILES "${CMAKE_CURRENT_LIST_DIR}/cmake/ports/*" "${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/*"
     "${CMAKE_CURRENT_LIST_DIR}/cmake/triplets/*" "${CMAKE_CURRENT_LIST_DIR}/vcpkg.json")

set(HASHES)

message("-- Hashing files")

foreach (FILE IN LISTS FILES)
    file(READ ${FILE} CONTENT)
    string(SHA256 HASH "${CONTENT}")
    file(RELATIVE_PATH REL_FILE ${CMAKE_CURRENT_LIST_DIR} ${FILE})
    message("        ${REL_FILE} -> ${HASH}")
    list(APPEND HASHES "${HASH}")
endforeach ()

message("-- Combined hash is")

string(SHA256 DEP_HASH "${HASHES}")
message(STATUS ${DEP_HASH})
