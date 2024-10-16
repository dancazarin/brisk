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
    brisk-renderer-d3d11
    D3D11Renderer.hpp
    RenderDevice.hpp
    RenderEncoder.hpp
    ImageRenderTarget.hpp
    WindowRenderTarget.hpp
    ImageBackend.hpp
    Common.hpp
    D3D11Renderer.cpp
    RenderDevice.cpp
    RenderEncoder.cpp
    ImageRenderTarget.cpp
    WindowRenderTarget.cpp
    ImageBackend.cpp
    Common.cpp)

foreach (SHADER_TYPE IN ITEMS "fragment" "vertex")
    set(SHADER_EXT fxc)
    set(FXC_MODE_fragment ps_5_0)
    set(FXC_MODE_vertex vs_5_0)

    find_program(FXC fxc.exe)

    if (NOT FXC)
        set(FXC fxc.exe) # Fallback for Visual Studio generator
    endif ()

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_TYPE}.fxc
        COMMAND
            ${FXC} -nologo -T ${FXC_MODE_${SHADER_TYPE}}
            # -WX
            -O3 -E ${SHADER_TYPE}Main -Fo ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_TYPE}.fxc
            ${CMAKE_CURRENT_LIST_DIR}/${SHADER_TYPE}.hlsl
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${SHADER_TYPE}.hlsl
        VERBATIM)

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${SHADER_TYPE}.fxc PROPERTIES GENERATED TRUE)

    brisk_target_link_resource(
        brisk-renderer-d3d11 PUBLIC shader_${SHADER_TYPE}
        INPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_TYPE}.fxc
        BROTLI)
endforeach ()

target_link_libraries(brisk-renderer-d3d11 PUBLIC brisk-core)

target_link_libraries(brisk-renderer-d3d11 PUBLIC d3d11 dxgi)
