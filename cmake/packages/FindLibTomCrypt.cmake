# Find the LibTomCrypt package
# LibTomCrypt_FOUND - Indicates whether LibTomCrypt was found
# LibTomCrypt_INCLUDE_DIR - The directory containing the LibTomCrypt headers
# LibTomCrypt_LIBRARIES - The LibTomCrypt libraries

find_path(LibTomCrypt_INCLUDE_DIR "tomcrypt.h")
find_library(LibTomCrypt_LIBRARIES tomcrypt)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibTomCrypt DEFAULT_MSG LibTomCrypt_LIBRARIES LibTomCrypt_INCLUDE_DIR)

mark_as_advanced(LibTomCrypt_INCLUDE_DIR LibTomCrypt_LIBRARY)
