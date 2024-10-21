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
# Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
# and a commercial license. You may use, modify, and distribute this software under
# the terms of the GPL-2.0+ license if you comply with its conditions.
#
# You should have received a copy of the GNU General Public License along with this program.
# If not, see <http://www.gnu.org/licenses/>.
#
# If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
# license. For commercial licensing options, please visit: https://brisklib.com
#

cmake_minimum_required(VERSION 3.16)

if (NOT BRISK_ROOT)
    set(BRISK_ROOT
        ${CMAKE_CURRENT_LIST_DIR}
        CACHE PATH "" FORCE)
endif ()

include(cmake/init.cmake)

project(brisk CXX C)

option(BRISK_LTO ON "Use Link Time Optimization in Release builds")
option(BRISK_BROTLI "Enable Brotli compression" ON)
if (NOT WIN32)
    set(ENABLE_WEBGPU ON)
else ()
    set(ENABLE_WEBGPU OFF)
endif ()
option(BRISK_WEBGPU "Enable WebGPU" ${ENABLE_WEBGPU})
option(BRISK_GONOTO "Include GoNoto font" OFF)
option(BRISK_LOG_TO_STDERR "Write log output to stderr (Windows-specific)" OFF)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(TESTS_AND_EXAMPLES ON)
else ()
    set(TESTS_AND_EXAMPLES OFF)
endif ()
option(BRISK_TESTS "Enable Brisk tests building" ${TESTS_AND_EXAMPLES})
option(BRISK_EXAMPLES "Enable Brisk examples building" ${TESTS_AND_EXAMPLES})

if (BRISK_LTO AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options($<$<CONFIG:Release>:-flto=thin>)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(-utf-8)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    add_link_options(/Map)
endif ()

set(DEPS_DIR
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    CACHE STRING "DEPS_DIR")

if (WIN32)
    set(PLATFORM_SPECIFIC_CPP "_Windows.cpp")
elseif (APPLE)
    set(PLATFORM_SPECIFIC_CPP "_Darwin.mm")
else ()
    set(PLATFORM_SPECIFIC_CPP "_Linux.cpp")
endif ()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/licenses.txt" "")

include(cmake/add_autotests.cmake)
include(cmake/compile_resources.cmake)
include(cmake/setup_executable.cmake)
include(cmake/metadata.cmake)

if (BRISK_LOG_TO_STDERR)
    add_compile_definitions(BRISK_LOG_TO_STDERR=1)
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include(cmake/add_license.cmake)

file(GLOB_RECURSE LICENSE_LIST ${DEPS_DIR}/share/copyright) # CONFIGURE_DEPENDS

foreach (license_path ${LICENSE_LIST})
    get_filename_component(PKG_PATH ${license_path} DIRECTORY)
    get_filename_component(PKG_NAME ${PKG_PATH} NAME)
    add_license(${PKG_NAME} ${license_path})
endforeach ()

set(RESOURCES_DIR ${PROJECT_SOURCE_DIR}/resources)
set(FONTS_DIR ${RESOURCES_DIR}/fonts)
set(IMAGES_DIR ${RESOURCES_DIR}/images)

add_subdirectory(src)

add_library(brisk-executable INTERFACE)
if (WIN32)
    target_sources(brisk-executable INTERFACE src/window/default.manifest)
endif ()

set(TESTS_GRAPHICS_LIBS)
set(TESTS_GRAPHICS_DEFS)

find_package(glfw3 CONFIG)
if (glfw3_FOUND)
    message(STATUS "GLFW3 is found. It will be used for interactive tests")
    list(APPEND TESTS_GRAPHICS_LIBS glfw)
    list(APPEND TESTS_GRAPHICS_DEFS HAVE_GLFW3)
else ()
    message(STATUS "GLFW3 is not found, skipping interactive tests")
endif ()

if (BRISK_TESTS)

    file(GLOB TESTS_CORE ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*_test.cpp)
    file(GLOB TESTS_NETWORK ${CMAKE_CURRENT_SOURCE_DIR}/src/network/*_test.cpp)
    file(GLOB TESTS_GRAPHICS ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/*_test.cpp)
    file(GLOB TESTS_WINDOW ${CMAKE_CURRENT_SOURCE_DIR}/src/window/*_test.cpp)
    file(GLOB TESTS_GUI ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/*_test.cpp)
    file(GLOB TESTS_WIDGETS ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/*_test.cpp)

    enable_testing()

    add_autotests(core SOURCES ${TESTS_CORE} LIBRARIES brisk-core)
    add_autotests(network SOURCES ${TESTS_NETWORK} LIBRARIES brisk-network)
    add_autotests(
        graphics
        SOURCES
        ${TESTS_GRAPHICS}
        LIBRARIES
        brisk-graphics
        ${TESTS_GRAPHICS_LIBS}
        DEFINITIONS
        ${TESTS_GRAPHICS_DEFS})
    add_autotests(window SOURCES ${TESTS_WINDOW} LIBRARIES brisk-window)
    add_autotests(gui SOURCES ${TESTS_GUI} LIBRARIES brisk-gui)
    add_autotests(widgets SOURCES ${TESTS_WIDGETS} LIBRARIES brisk-widgets)

endif ()

if (BRISK_EXAMPLES)

    add_subdirectory(examples)

endif ()
