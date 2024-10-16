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
    brisk-graphics-vector STATIC
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vbezier.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vdasher.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vmatrix.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vpath.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vpathmesure.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vraster.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vrect.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vrle.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype/v_ft_math.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype/v_ft_raster.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype/v_ft_stroker.cpp)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(brisk-graphics-vector PRIVATE -Wno-deprecated-volatile)
endif ()
target_compile_definitions(brisk-graphics-vector PRIVATE V_NAMESPACE=Brisk)
target_include_directories(brisk-graphics-vector PUBLIC ${PROJECT_SOURCE_DIR}/src/graphics/vector)
target_include_directories(brisk-graphics-vector PRIVATE ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype)

add_library(
    brisk-graphics STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Renderer.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/ImageFormats.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/ImageTransform.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Color.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Matrix.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/RawCanvas.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Canvas.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/SVG.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/RenderState.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Fonts.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Path.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Offscreen.hpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ICU.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Gradients.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/SVG.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_png.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_jpeg.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_bmp.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_webp.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageTransform.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/RawCanvas.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Canvas.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/RenderState.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/OpenType.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/FlatAllocator.hpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Atlas.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Atlas.hpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Fonts.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/SingleHeaderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Image.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Path.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Renderer.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Offscreen.cpp)

if (WIN32)
    add_subdirectory(D3D11Renderer)
    target_link_libraries(brisk-graphics PRIVATE brisk-renderer-d3d11)
endif ()

if (BRISK_WEBGPU)
    add_subdirectory(WebGPURenderer)
    if (TARGET brisk-renderer-webgpu)
        target_link_libraries(brisk-graphics PRIVATE brisk-renderer-webgpu)
        target_compile_definitions(brisk-graphics PUBLIC BRISK_WEBGPU=1)
    else ()
        message(FATAL_ERROR "BRISK_WEBGPU is ON but brisk-renderer-webgpu target is missing")
    endif ()
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (MSVC)
        set_source_files_properties(${PROJECT_SOURCE_DIR}/src/graphics/Fonts.cpp PROPERTIES COMPILE_FLAGS /fp:precise)
    else ()
        set_source_files_properties(${PROJECT_SOURCE_DIR}/src/graphics/Fonts.cpp PROPERTIES COMPILE_FLAGS
                                                                                            -fno-fast-math)
    endif ()
endif ()

target_compile_definitions(brisk-graphics PRIVATE V_NAMESPACE=Brisk)
target_link_libraries(brisk-graphics PRIVATE brisk-graphics-vector)

target_include_directories(brisk-graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(brisk-graphics PUBLIC brisk-core)

# >PNG
find_package(PNG REQUIRED)
target_link_libraries(brisk-graphics PRIVATE PNG::PNG)
# /PNG

# >WebP
find_package(WebP CONFIG REQUIRED)
target_link_libraries(brisk-graphics PRIVATE WebP::webp WebP::webpdecoder)
# /WebP

# >libjpeg-turbo
find_package(libjpeg-turbo CONFIG REQUIRED)
target_link_libraries(
    brisk-graphics
    PRIVATE $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>)
# /libjpeg-turbo

# >lunasvg
find_package(unofficial-lunasvg CONFIG REQUIRED)
target_link_libraries(brisk-graphics PRIVATE unofficial::lunasvg::lunasvg)
# /lunasvg

# >harfbuzz
find_package(harfbuzz CONFIG REQUIRED)
target_link_libraries(brisk-graphics PRIVATE harfbuzz)
# /harfbuzz

# >freetype
find_package(freetype CONFIG REQUIRED)
target_link_libraries(brisk-graphics PRIVATE freetype)
# /freetype

# >icu
find_package(ICU COMPONENTS uc)
if (TARGET ICU::uc)

    file(SIZE "${PROJECT_SOURCE_DIR}/resources/icu/icudt74l.dat" ICUDT_SIZE)
    brisk_target_link_resource(
        brisk-graphics PRIVATE icudt
        INPUT "${PROJECT_SOURCE_DIR}/resources/icu/icudt74l.dat"
        BROTLI)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/src/graphics/ICU.cpp PROPERTIES COMPILE_DEFINITIONS
                                                                                      ICUDT_SIZE=${ICUDT_SIZE})

    target_link_libraries(brisk-graphics PRIVATE ICU::uc)
endif ()
# /icu

set_target_properties(brisk-graphics PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")

if (APPLE)
    target_link_libraries(brisk-graphics PUBLIC "-framework Metal" "-framework IOKit" "-framework QuartzCore"
                                                "-framework CoreGraphics" "-framework CoreVideo")
endif ()
