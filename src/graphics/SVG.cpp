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
#include <brisk/graphics/SVG.hpp>
#include <lunasvg.h>

namespace Brisk {

constexpr static PixelFormat lunaFormat = PixelFormat::BGRA;

namespace Internal {
struct SVGImpl : public lunasvg::Document {
public:
    using lunasvg::Document::Document;
};
} // namespace Internal

using Internal::SVGImpl;

SVGImage::SVGImage(std::string_view svg) {
    m_impl.reset(
        reinterpret_cast<SVGImpl*>(lunasvg::Document::loadFromData(svg.data(), svg.size()).release()));
}

SVGImage::SVGImage(bytes_view svg) : SVGImage(toStringView(svg)) {}

SVGImage::~SVGImage() = default;

RC<Image> SVGImage::render(Size size, ColorF background) const {
    Color color         = background;
    lunasvg::Bitmap bmp = m_impl->renderToBitmap(size.width, size.height, std::bit_cast<uint32_t>(color));
    RC<Image> image     = rcnew Image(size, ImageFormat::RGBA);
    convertPixels(image->pixelFormat(), image->data().to<uint8_t>(), lunaFormat,
                  StridedData<const uint8_t>{
                      reinterpret_cast<const uint8_t*>(bmp.data()),
                      int32_t(bmp.stride()),
                  },
                  size);
    return image;
}

} // namespace Brisk
