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
set(MAGICK_VERSION 7.1.1-29)
set(MAGICK_EXTRACT_DIR ${CMAKE_CURRENT_BINARY_DIR}/Magick)

file(
    DOWNLOAD
    https://github.com/ImageMagick/ImageMagick/releases/download/${MAGICK_VERSION}/ImageMagick.Q16-HDRI.msixbundle
    ${CMAKE_CURRENT_BINARY_DIR}/ImageMagick-${MAGICK_VERSION}.msixbundle)

file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/ImageMagick-${MAGICK_VERSION}.msixbundle DESTINATION
     ${MAGICK_EXTRACT_DIR})
file(ARCHIVE_EXTRACT INPUT ${MAGICK_EXTRACT_DIR}/main-x64.appx DESTINATION ${MAGICK_EXTRACT_DIR}/x64-app)
# # file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/${MAGICK_FILE} DESTINATION ${MAGICK_EXTRACT_DIR})

find_program(
    MAGICK_EXECUTABLE
    NAMES magick
    PATHS ${MAGICK_EXTRACT_DIR}/x64-app REQUIRED
    NO_DEFAULT_PATH)
