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

add_license("Lato fonts" ${FONTS_DIR}/Lato-OFL.txt)
add_license("SourceCodePro" ${FONTS_DIR}/SourceCodePro-LICENSE.txt)
add_license("Lucide Icons" ${FONTS_DIR}/Lucide-Icons-LICENSE.txt)

add_library(
    brisk-gui STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/GUIWindow.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/GUIOffscreen.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/Icons.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/GUI.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/WidgetTree.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/Groups.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/Component.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/gui/GUIApplication.hpp
    ${PROJECT_SOURCE_DIR}/src/gui/GUIWindow.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/GUI.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/WidgetTree.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Styles.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/GUIOffscreen.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Event.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Animation.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Groups.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/SingleHeaderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Layout.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Component.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/GUIApplication.cpp)

add_subdirectory(yoga)

brisk_target_link_resource(
    brisk-gui PRIVATE Lato-Black
    INPUT "${FONTS_DIR}/Lato-Black.ttf"
    BROTLI)
brisk_target_link_resource(
    brisk-gui PRIVATE Lato-Medium
    INPUT "${FONTS_DIR}/Lato-Medium.ttf"
    BROTLI)
brisk_target_link_resource(
    brisk-gui PRIVATE Lato-Light
    INPUT "${FONTS_DIR}/Lato-Light.ttf"
    BROTLI)
brisk_target_link_resource(
    brisk-gui PRIVATE SourceCodePro-Medium
    INPUT "${FONTS_DIR}/SourceCodePro-Medium.ttf"
    BROTLI)
if (BRISK_GONOTO)
    brisk_target_link_resource(
        brisk-gui PRIVATE GoNotoCurrent-Regular
        INPUT "${FONTS_DIR}/GoNotoCurrent-Regular.ttf"
        BROTLI)
endif ()
brisk_target_link_resource(brisk-gui PUBLIC brisk-white INPUT "${RESOURCES_DIR}/images/brisk-white.png")
brisk_target_link_resource(
    brisk-gui PUBLIC brisk
    INPUT "${RESOURCES_DIR}/images/brisk.svg"
    BROTLI)
brisk_target_link_resource(
    brisk-gui PRIVATE Lucide
    INPUT "${FONTS_DIR}/Lucide.ttf"
    BROTLI)

target_link_libraries(brisk-gui PUBLIC brisk-window yogacore)

set_target_properties(brisk-gui PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")
