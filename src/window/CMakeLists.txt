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
    brisk-window STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/window/WindowApplication.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/window/Types.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/window/Window.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/window/OSDialogs.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/window/Clipboard.hpp
    ${PROJECT_SOURCE_DIR}/src/window/PlatformWindow.hpp
    ${PROJECT_SOURCE_DIR}/src/window/WindowApplication.cpp
    ${PROJECT_SOURCE_DIR}/src/window/PlatformWindow.cpp
    ${PROJECT_SOURCE_DIR}/src/window/Window.cpp
    ${PROJECT_SOURCE_DIR}/src/window/OSDialogs${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/window/SingleHeaderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/window/Types.cpp
    ${PROJECT_SOURCE_DIR}/src/window/PlatformWindow${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/window/Clipboard${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/window/Display${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/window/KeyCodes${PLATFORM_SPECIFIC_CPP})

target_link_libraries(brisk-window PUBLIC brisk-graphics)

if (WIN32)
    target_link_libraries(brisk-window PUBLIC dwmapi Shlwapi Comctl32 Shcore)
endif ()
if (APPLE)
    target_link_libraries(brisk-window PUBLIC "-framework Cocoa" "-framework UniformTypeIdentifiers")
endif ()
if (LINUX)
    find_package(X11 REQUIRED)
    target_link_libraries(brisk-window PUBLIC X11::X11 X11::Xrandr X11::Xinerama)
endif ()

set_target_properties(brisk-window PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")
