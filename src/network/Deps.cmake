# >CURL
find_package(CURL REQUIRED)
target_link_libraries(brisk-network ${_DEP_PRIVATE} CURL::libcurl)
# /CURL
