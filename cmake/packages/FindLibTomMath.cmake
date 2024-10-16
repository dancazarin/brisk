# Find the LibTomMath package
# LibTomMath_FOUND - Indicates whether LibTomMath was found
# LibTomMath_INCLUDE_DIR - The directory containing the LibTomMath headers
# LibTomMath_LIBRARIES - The LibTomMath libraries

find_path(LibTomMath_INCLUDE_DIR "tommath.h")
find_library(LibTomMath_LIBRARIES tommath)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibTomMath DEFAULT_MSG LibTomMath_LIBRARIES LibTomMath_INCLUDE_DIR)

mark_as_advanced(LibTomMath_INCLUDE_DIR LibTomMath_LIBRARY)
