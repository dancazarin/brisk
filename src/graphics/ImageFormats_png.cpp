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
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Utilities.hpp>

#include <png.h>

namespace Brisk {

static PixelFormat fromPNGFormat(png_uint_32 fmt) {
    return staticMap(fmt,                                        //
                     PNG_FORMAT_RGB, PixelFormat::RGB,           //
                     PNG_FORMAT_RGBA, PixelFormat::RGBA,         //
                     PNG_FORMAT_ARGB, PixelFormat::ARGB,         //
                     PNG_FORMAT_BGR, PixelFormat::BGR,           //
                     PNG_FORMAT_BGRA, PixelFormat::BGRA,         //
                     PNG_FORMAT_ABGR, PixelFormat::ABGR,         //
                     PNG_FORMAT_GA, PixelFormat::GreyscaleAlpha, //
                     PNG_FORMAT_AG, PixelFormat::Unknown,        //
                     PixelFormat::Greyscale);
}

static png_uint_32 toPNGFormat(PixelFormat fmt) {
    return staticMap(fmt,                                        //
                     PixelFormat::RGB, PNG_FORMAT_RGB,           //
                     PixelFormat::RGBA, PNG_FORMAT_RGBA,         //
                     PixelFormat::ARGB, PNG_FORMAT_ARGB,         //
                     PixelFormat::BGR, PNG_FORMAT_BGR,           //
                     PixelFormat::BGRA, PNG_FORMAT_BGRA,         //
                     PixelFormat::ABGR, PNG_FORMAT_ABGR,         //
                     PixelFormat::GreyscaleAlpha, PNG_FORMAT_GA, //
                     PNG_FORMAT_GRAY);
}

bytes pngEncode(RC<Image> image) {
    // All png pixel formats are supported except PNG_FORMAT_AG
    png_image pngimage;
    memset(&pngimage, 0, sizeof(pngimage));
    pngimage.version = PNG_IMAGE_VERSION;
    pngimage.width   = image->width();
    pngimage.height  = image->height();
    pngimage.format  = toPNGFormat(image->format());
    bytes b(PNG_IMAGE_PNG_SIZE_MAX(pngimage), 0);
    png_alloc_size_t pngbytes = b.size();

    SCOPE_EXIT {
        png_image_free(&pngimage);
    };

    auto r = image->mapRead();
    if (png_image_write_to_memory(&pngimage, b.data(), &pngbytes, 0, r.data(), r.byteStride(), nullptr)) {
        b.resize(pngbytes);
        return b;
    } else {
        return {};
    }
}

expected<RC<Image>, ImageIOError> pngDecode(bytes_view bytes, PixelFormat format) {
    png_image pngimage;
    memset(&pngimage, 0, sizeof(pngimage));
    pngimage.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&pngimage, bytes.data(), bytes.size())) {
        return unexpected(ImageIOError::CodecError);
    }
    if (format != PixelFormat::Unknown) {
        pngimage.format = toPNGFormat(format);
    } else {
        format = fromPNGFormat(pngimage.format);
        if (format == PixelFormat::Unknown) {
            return unexpected(ImageIOError::InvalidFormat);
        }
    }
    RC<Image> image = createImage(Size(pngimage.width, pngimage.height), format);
    auto w          = image->mapWrite();
    if (!png_image_finish_read(&pngimage, nullptr, w.data(), w.byteStride(), nullptr)) {
        return unexpected(ImageIOError::CodecError);
    }
    return image;
}

} // namespace Brisk
