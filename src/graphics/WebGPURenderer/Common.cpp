/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include "Common.hpp"

namespace Brisk {
wgpu::TextureFormat wgFormat(PixelType type, PixelFormat format) {
    constexpr wgpu::TextureFormat _ = wgpu::TextureFormat::Undefined;
    if (+type >= enumSize<PixelType>)
        return _;
    if (+format >= enumSize<PixelFormat>)
        return _;
    using enum wgpu::TextureFormat;
    constexpr static wgpu::TextureFormat formats[enumSize<PixelFormat>][enumSize<PixelType>]{
        // clang-format off
/*                     U8                          U8Gamma                          U16                             F32                  */
/* RGB,            */ {_,                          _,                               _,                              _                             },
/* RGBA,           */ {RGBA8Unorm,                 RGBA8UnormSrgb,                  RGBA16Unorm,                    RGBA32Float                   },
/* ARGB,           */ {_,                          _,                               _,                              _                             },
/* BGR,            */ {_,                          _,                               _,                              _                             },
/* BGRA,           */ {BGRA8Unorm,                 BGRA8UnormSrgb,                  _,                              _                             },
/* ABGR,           */ {_,                          _,                               _,                              _                             },
/* GreyscaleAlpha, */ {RG8Unorm,                   _,                               RG16Unorm,                      RG32Float                     },
/* Greyscale,      */ {R8Unorm,                    _,                               R16Unorm,                       R32Float                      },
/* Alpha,          */ {R8Unorm,                    _,                               _,                              _                             },
        // clang-format on
    };
    return formats[+format][+type];
}

} // namespace Brisk
