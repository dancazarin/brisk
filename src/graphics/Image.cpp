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
#include <brisk/graphics/Image.hpp>

namespace Brisk {

namespace {

template <typename T>
using cvtPixelsFn = void (*)(T* dst, const T* src, uint32_t size);

template <PixelFormat fmt>
constexpr std::nullptr_t pixFmtOrder4{};
template <PixelFormat fmt>
constexpr std::nullptr_t pixFmtOrder3{};

template <typename T, size_t N, size_t... X>
BRISK_INLINE SIMD<T, N> permute(const SIMD<T, N>& value, size_constants<X...> indices)
    requires(N % sizeof...(X) == 0 && std::has_single_bit(N / sizeof...(X)))
{
    if constexpr (sizeof...(X) < N) {
        return permute(value, size_constants<X..., (sizeof...(X) + X)...>{});
    } else if constexpr (sizeof...(X) == N) {
        return value.shuffle(indices);
    }
}

template <>
constexpr size_constants<0, 1, 2, 3> pixFmtOrder4<PixelFormat::RGBA>{};
template <>
constexpr size_constants<1, 2, 3, 0> pixFmtOrder4<PixelFormat::ARGB>{};
template <>
constexpr size_constants<2, 1, 0, 3> pixFmtOrder4<PixelFormat::BGRA>{};
template <>
constexpr size_constants<3, 2, 1, 0> pixFmtOrder4<PixelFormat::ABGR>{};

template <>
constexpr size_constants<0, 1, 2> pixFmtOrder3<PixelFormat::RGB>{};
template <>
constexpr size_constants<2, 1, 0> pixFmtOrder3<PixelFormat::BGR>{};

template <PixelFormat dstFmt, PixelFormat srcFmt, typename T>
void cvtPixels(T* dst, const T* src, uint32_t size) {
    if constexpr (srcFmt == dstFmt) {
        memcpy(dst, src, size * pixelComponents(dstFmt));
    } else if constexpr (pixelComponents(dstFmt) == 4 && pixelComponents(srcFmt) == 4) {
        // reorder

        constexpr size_t N = 4;

        int32_t x          = 0;
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x + N - 1 < size; x += N) {
            SIMD<T, 4 * N> t = SIMD<T, 4 * N>::read(src);
            t                = permute(permute(t, pixFmtOrder4<srcFmt>), pixFmtOrder4<dstFmt>);
            t.write(dst);
            src += 4 * N;
            dst += 4 * N;
        }
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x < size; ++x) {
            SIMD<T, 4> t = SIMD<T, 4>::read(src);
            t            = permute(permute(t, pixFmtOrder4<srcFmt>), pixFmtOrder4<dstFmt>);
            t.write(dst);
            src += 4;
            dst += 4;
        }

    } else if constexpr (pixelComponents(dstFmt) == 3 && pixelComponents(srcFmt) == 3) {

        constexpr size_t N = 4;

        int32_t x          = 0;
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x + N - 1 < size; x += N) {
            SIMD<T, 3 * N> t = SIMD<T, 3 * N>::read(src);
            t                = permute(permute(t, pixFmtOrder3<srcFmt>), pixFmtOrder3<dstFmt>);
            t.write(dst);
            src += 3 * N;
            dst += 3 * N;
        }
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x < size; ++x) {
            SIMD<T, 3> t = SIMD<T, 3>::read(src);
            t            = permute(permute(t, pixFmtOrder3<srcFmt>), pixFmtOrder3<dstFmt>);
            t.write(dst);
            src += 3;
            dst += 3;
        }
    } else {
        Pixel<T, dstFmt>* dest         = reinterpret_cast<Pixel<T, dstFmt>*>(dst);
        const Pixel<T, srcFmt>* source = reinterpret_cast<const Pixel<T, srcFmt>*>(src);
        for (int32_t x = 0; x < size; ++x) {
            dest[x] = cvtPixel<dstFmt>(source[x]);
        }
    }
}

constexpr int numPixelFormats = std::size(pixelFormatDesc);

template <typename T> //          dst              src
const cvtPixelsFn<T> cvtTable[numPixelFormats][numPixelFormats]{
    /* RGB  */ {
        &cvtPixels<PixelFormat::RGB, PixelFormat::RGB>, // reorder
        &cvtPixels<PixelFormat::RGB, PixelFormat::RGBA>,
        &cvtPixels<PixelFormat::RGB, PixelFormat::ARGB>,
        &cvtPixels<PixelFormat::RGB, PixelFormat::BGR>, // reorder
        &cvtPixels<PixelFormat::RGB, PixelFormat::BGRA>,
        &cvtPixels<PixelFormat::RGB, PixelFormat::ABGR>,
        &cvtPixels<PixelFormat::RGB, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::RGB, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::RGB, PixelFormat::Alpha>,
    },
    /* RGBA */
    {
        &cvtPixels<PixelFormat::RGBA, PixelFormat::RGB>,
        &cvtPixels<PixelFormat::RGBA, PixelFormat::RGBA>, // reorder
        &cvtPixels<PixelFormat::RGBA, PixelFormat::ARGB>, // reorder
        &cvtPixels<PixelFormat::RGBA, PixelFormat::BGR>,
        &cvtPixels<PixelFormat::RGBA, PixelFormat::BGRA>, // reorder
        &cvtPixels<PixelFormat::RGBA, PixelFormat::ABGR>, // reorder
        &cvtPixels<PixelFormat::RGBA, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::RGBA, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::RGBA, PixelFormat::Alpha>,
    },
    /* ARGB */
    {
        &cvtPixels<PixelFormat::ARGB, PixelFormat::RGB>,
        &cvtPixels<PixelFormat::ARGB, PixelFormat::RGBA>, // reorder
        &cvtPixels<PixelFormat::ARGB, PixelFormat::ARGB>, // reorder
        &cvtPixels<PixelFormat::ARGB, PixelFormat::BGR>,
        &cvtPixels<PixelFormat::ARGB, PixelFormat::BGRA>, // reorder
        &cvtPixels<PixelFormat::ARGB, PixelFormat::ABGR>, // reorder
        &cvtPixels<PixelFormat::ARGB, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::ARGB, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::ARGB, PixelFormat::Alpha>,
    },
    /* BGR  */
    {
        &cvtPixels<PixelFormat::BGR, PixelFormat::RGB>, // reorder
        &cvtPixels<PixelFormat::BGR, PixelFormat::RGBA>,
        &cvtPixels<PixelFormat::BGR, PixelFormat::ARGB>,
        &cvtPixels<PixelFormat::BGR, PixelFormat::BGR>, // reorder
        &cvtPixels<PixelFormat::BGR, PixelFormat::BGRA>,
        &cvtPixels<PixelFormat::BGR, PixelFormat::ABGR>,
        &cvtPixels<PixelFormat::BGR, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::BGR, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::BGR, PixelFormat::Alpha>,
    },
    /* BGRA */
    {
        &cvtPixels<PixelFormat::BGRA, PixelFormat::RGB>,
        &cvtPixels<PixelFormat::BGRA, PixelFormat::RGBA>, // reorder
        &cvtPixels<PixelFormat::BGRA, PixelFormat::ARGB>, // reorder
        &cvtPixels<PixelFormat::BGRA, PixelFormat::BGR>,
        &cvtPixels<PixelFormat::BGRA, PixelFormat::BGRA>, // reorder
        &cvtPixels<PixelFormat::BGRA, PixelFormat::ABGR>, // reorder
        &cvtPixels<PixelFormat::BGRA, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::BGRA, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::BGRA, PixelFormat::Alpha>,
    },
    /* ABGR */
    {
        &cvtPixels<PixelFormat::ABGR, PixelFormat::RGB>,
        &cvtPixels<PixelFormat::ABGR, PixelFormat::RGBA>, // reorder
        &cvtPixels<PixelFormat::ABGR, PixelFormat::ARGB>, // reorder
        &cvtPixels<PixelFormat::ABGR, PixelFormat::BGR>,
        &cvtPixels<PixelFormat::ABGR, PixelFormat::BGRA>, // reorder
        &cvtPixels<PixelFormat::ABGR, PixelFormat::ABGR>, // reorder
        &cvtPixels<PixelFormat::ABGR, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::ABGR, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::ABGR, PixelFormat::Alpha>,
    },
    /* GreyscaleAlpha */
    {
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::RGB>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::RGBA>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::ARGB>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::BGR>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::BGRA>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::ABGR>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::GreyscaleAlpha, PixelFormat::Alpha>,
    },
    /* Greyscale */
    {
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::RGB>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::RGBA>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::ARGB>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::BGR>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::BGRA>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::ABGR>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::Greyscale, PixelFormat::Alpha>,
    },
    /* A   */
    {
        &cvtPixels<PixelFormat::Alpha, PixelFormat::RGB>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::RGBA>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::ARGB>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::BGR>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::BGRA>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::ABGR>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::Greyscale>,
        &cvtPixels<PixelFormat::Alpha, PixelFormat::Alpha>,
    },
};

} // namespace

void convertPixels(PixelFormat dstFmt, StridedData<uint8_t> dst, PixelFormat srcFmt,
                   StridedData<const uint8_t> src, Size size) {
    cvtPixelsFn<uint8_t> fn = cvtTable<uint8_t>[+dstFmt][+srcFmt];
    for (int32_t y = 0; y < size.height; ++y) {
        fn(dst.line(y), src.line(y), size.width);
    }
}
} // namespace Brisk
