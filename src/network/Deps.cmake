get_property(
    _BRISK_NETWORK
    TARGET brisk-network
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_NETWORK}" STREQUAL "")
    set(_BRISK_NETWORK brisk-network)
endif ()

# >CURL
find_package(CURL REQUIRED)
target_link_libraries(${_BRISK_NETWORK} ${_DEP_PRIVATE} CURL::libcurl)
# /CURL
