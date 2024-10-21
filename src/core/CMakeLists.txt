#
# Brisk
#
# Cross-platform application framework
# --------------------------------------------------------------
#
# Copyright (C) 2024 Brisk Developers
#
# This file is part of the Brisk library.
#
# Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+), and a commercial license. You may
# use, modify, and distribute this software under the terms of the GPL-2.0+ license if you comply with its conditions.
#
# You should have received a copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
#
# If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial license. For commercial
# licensing options, please visit: https://brisklib.com
#

add_library(
    brisk-core STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/core/BasicTypes.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Json.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Bytes.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Compression.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Cryptography.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Hash.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/CRCcompile.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Encoding.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Expected.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Generation.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/IO.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Utilities.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Fixed.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Log.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Optional.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Serialization.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Settings.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Text.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Threading.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Time.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Math.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Initialization.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Functional.hpp
    ${PROJECT_SOURCE_DIR}/src/core/Json.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Binding.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression_Brotli.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression_GZip.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression_LZ4.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Cryptography.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Exceptions.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Hash.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Encoding.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Process.cpp
    ${PROJECT_SOURCE_DIR}/src/core/IO.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Log.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Settings.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Embed.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Text.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Localization.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Threading.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Stream.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Time.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Initialization.cpp
    ${PROJECT_SOURCE_DIR}/src/core/SingleHeaderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/core/uv.hpp
    ${PROJECT_SOURCE_DIR}/src/core/uv.cpp
    ${PROJECT_SOURCE_DIR}/src/core/App.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Version.cpp
    ${PROJECT_SOURCE_DIR}/src/core/cityhash/city.cc
    ${PROJECT_SOURCE_DIR}/src/core/cityhash/city.h
    ${PROJECT_SOURCE_DIR}/src/core/llvm/SmallVector.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Throw.cpp
    ${PROJECT_SOURCE_DIR}/src/core/System.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Threading${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/System${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/IO${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/DynamicLibrary${PLATFORM_SPECIFIC_CPP})

if (WIN32)
    target_sources(brisk-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/COMInit.cpp)
endif ()
if (APPLE)
    target_sources(brisk-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/AutoreleasePool.mm)
endif ()

target_include_directories(brisk-core PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_include_directories(brisk-core PUBLIC ${DEPS_DIR}/include/Common)

if (WIN32)
    target_compile_definitions(brisk-core PUBLIC _CRT_SECURE_NO_WARNINGS=1)
    target_compile_definitions(brisk-core PUBLIC _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1)
    target_compile_definitions(brisk-core PUBLIC _ENABLE_EXTENDED_ALIGNED_STORAGE=1)
    target_compile_definitions(brisk-core PUBLIC -DUNICODE)
    target_compile_definitions(brisk-core PUBLIC -DWINVER=0x0A00)
    target_compile_definitions(brisk-core PUBLIC -DWIN32=1)
    target_compile_definitions(brisk-core PUBLIC -D_WIN32_WINNT=0xA00)
    target_compile_definitions(brisk-core PUBLIC -DNOMINMAX)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(brisk-core PUBLIC -Werror=unused-result)
    target_compile_options(brisk-core PUBLIC -Werror=return-type)
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(brisk-core PUBLIC -Wno-attributes)
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(brisk-core PUBLIC -Werror=microsoft-template)
    target_compile_options(brisk-core PUBLIC -fdiagnostics-absolute-paths)
    target_compile_options(brisk-core PUBLIC -Werror=infinite-recursion)
    target_compile_options(brisk-core PUBLIC -fdiagnostics-absolute-paths)
    target_compile_options(brisk-core PUBLIC -Werror=infinite-recursion)
    target_compile_options(brisk-core PUBLIC -Werror=literal-conversion)
endif ()

# >libuv
find_package(libuv CONFIG REQUIRED)
target_link_libraries(brisk-core PRIVATE $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
# /libuv

# >readerwriterqueue
find_path(READERWRITERQUEUE_INCLUDE_DIRS "readerwriterqueue/atomicops.h")
target_include_directories(brisk-core PRIVATE ${READERWRITERQUEUE_INCLUDE_DIRS})
# /readerwriterqueue

# >concurrentqueue
find_package(unofficial-concurrentqueue CONFIG REQUIRED)
target_link_libraries(brisk-core PRIVATE unofficial::concurrentqueue::concurrentqueue)
# /concurrentqueue

# >stb
find_package(Stb REQUIRED)
target_include_directories(brisk-core PRIVATE ${Stb_INCLUDE_DIR})
# /stb

# >fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(brisk-core PUBLIC fmt::fmt)
# /fmt

# >spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(brisk-core PUBLIC spdlog::spdlog)
# /spdlog

if (BRISK_BROTLI)
    # >brotli
    find_package(unofficial-brotli CONFIG REQUIRED)
    target_link_libraries(brisk-core PRIVATE unofficial::brotli::brotlidec unofficial::brotli::brotlienc)
    # /brotli

    target_compile_definitions(brisk-core PUBLIC BRISK_HAVE_BROTLI=1)
endif ()

# >libtomcrypt
find_package(LibTomCrypt REQUIRED)
target_link_libraries(brisk-core PUBLIC ${LibTomCrypt_LIBRARIES})
target_include_directories(brisk-core PRIVATE ${LibTomCrypt_INCLUDE_DIR})
# /libtomcrypt

# >libtommath
find_package(LibTomMath REQUIRED)
target_link_libraries(brisk-core PUBLIC ${LibTomMath_LIBRARIES})
target_include_directories(brisk-core PRIVATE ${LibTomMath_INCLUDE_DIR})
target_compile_definitions(brisk-core PUBLIC -DLTM_DESC -DUSE_LTM)
# /libtommath

# >rapidjson
find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(brisk-core PRIVATE rapidjson)
# /rapidjson

# >lz4
find_package(lz4 CONFIG REQUIRED)
target_link_libraries(brisk-core PRIVATE lz4::lz4)
# /lz4

# >msgpack-cxx
find_package(msgpack-cxx CONFIG REQUIRED)
target_link_libraries(brisk-core PRIVATE msgpack-cxx)
# /msgpack-cxx

# >utf8proc
find_package(unofficial-utf8proc CONFIG REQUIRED)
target_link_libraries(brisk-core PRIVATE utf8proc)
# /utf8proc

# >zlib
find_package(ZLIB REQUIRED)
target_link_libraries(brisk-core PRIVATE ZLIB::ZLIB)
# /zlib

# >tl-expected
find_package(tl-expected CONFIG REQUIRED)
target_link_libraries(brisk-core PRIVATE tl::expected)
# /tl-expected

if (WIN32)
    target_link_options(brisk-core PUBLIC /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/types.natvis)
endif ()

target_compile_definitions(brisk-core PUBLIC BRISK_CFG_$<CONFIG>=1)

add_executable(bin2c bin2c.cpp)
target_link_libraries(bin2c PRIVATE brisk-core)
set_target_properties(bin2c PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# System libraries and frameworks
if (WIN32)
    target_link_libraries(brisk-core PUBLIC ole32 uuid)
endif ()

if (APPLE)
    target_link_libraries(brisk-core PUBLIC "-framework Foundation")
    target_compile_options(brisk-core PUBLIC -fobjc-arc)
endif ()

set_target_properties(brisk-core PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(brisk-core PUBLIC -wd4003)
endif ()
