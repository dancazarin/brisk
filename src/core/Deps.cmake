# >libuv
find_package(libuv CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE}
                      $<BUILD_INTERFACE:$<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>>)
# /libuv

# >readerwriterqueue
find_path(READERWRITERQUEUE_INCLUDE_DIRS "readerwriterqueue/atomicops.h")
target_include_directories(brisk-core ${_DEP_PRIVATE} ${READERWRITERQUEUE_INCLUDE_DIRS})
# /readerwriterqueue

# >concurrentqueue
find_package(unofficial-concurrentqueue CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} unofficial::concurrentqueue::concurrentqueue)
# /concurrentqueue

# >stb
find_package(Stb REQUIRED)
target_include_directories(brisk-core ${_DEP_PRIVATE} ${Stb_INCLUDE_DIR})
# /stb

# >fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PUBLIC} fmt::fmt)
# /fmt

# >spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PUBLIC} spdlog::spdlog)
# /spdlog

# >brotli
find_package(unofficial-brotli CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} unofficial::brotli::brotlidec unofficial::brotli::brotlienc)
# /brotli

target_compile_definitions(brisk-core ${_DEP_PUBLIC} BRISK_HAVE_BROTLI=1)

# >libtommath
add_library(LibTom::Math UNKNOWN IMPORTED)

find_path(LibTomMath_INCLUDE_DIR "tommath.h")
find_library(
    LibTomMath_LIBRARIES_DEBUG
    NAMES tommath
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug"
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)
find_library(
    LibTomMath_LIBRARIES_RELEASE
    NAMES tommath
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)
if (NOT LibTomMath_LIBRARIES_DEBUG)
    set_target_properties(
        LibTom::Math
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LibTomMath_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "RELEASE"
            IMPORTED_LOCATION_RELEASE "${LibTomMath_LIBRARIES_RELEASE}")
else ()
    set_target_properties(
        LibTom::Math
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LibTomMath_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION_RELEASE "${LibTomMath_LIBRARIES_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${LibTomMath_LIBRARIES_DEBUG}")
endif ()

target_link_libraries(brisk-core ${_DEP_PRIVATE} LibTom::Math)
# /libtommath

# >libtomcrypt
add_library(LibTom::Crypt UNKNOWN IMPORTED)

find_path(LibTomCrypt_INCLUDE_DIR "tomcrypt.h")
find_library(
    LibTomCrypt_LIBRARIES_DEBUG
    NAMES tomcrypt
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug"
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)
find_library(
    LibTomCrypt_LIBRARIES_RELEASE
    NAMES tomcrypt
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)
if (NOT LibTomCrypt_LIBRARIES_DEBUG)
    set_target_properties(
        LibTom::Crypt
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LibTomCrypt_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "RELEASE"
            IMPORTED_LOCATION_RELEASE "${LibTomCrypt_LIBRARIES_RELEASE}")
else ()
    set_target_properties(
        LibTom::Crypt
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LibTomCrypt_INCLUDE_DIR}"
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
            IMPORTED_LOCATION_RELEASE "${LibTomCrypt_LIBRARIES_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${LibTomCrypt_LIBRARIES_DEBUG}")
endif ()

target_link_libraries(brisk-core ${_DEP_PRIVATE} LibTom::Crypt)

# /libtomcrypt

# >rapidjson
find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} rapidjson)
# /rapidjson

# >lz4
find_package(lz4 CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} lz4::lz4)
# /lz4

# >msgpack-cxx
find_package(msgpack-cxx CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} msgpack-cxx)
# /msgpack-cxx

# >utf8proc
find_package(unofficial-utf8proc CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} utf8proc)
# /utf8proc

# >zlib
find_package(ZLIB REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} ZLIB::ZLIB)
# /zlib

# >tl-expected
find_package(tl-expected CONFIG REQUIRED)
target_link_libraries(brisk-core ${_DEP_PRIVATE} tl::expected)
# /tl-expected
