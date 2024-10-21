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
#pragma once

#include <brisk/gui/GUI.hpp>
#include <brisk/graphics/SVG.hpp>
#include <brisk/graphics/ImageFormats.hpp>

namespace Brisk {

class WIDGET ImageView : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "imageview";

    template <WidgetArgument... Args>
    ImageView(bytes_view image, const Args&... args)
        : ImageView(Construction{ widgetType }, imageDecode(image, ImageFormat::RGBA).value(),
                    std::tuple{ args... }) {
        endConstruction();
    }

    template <WidgetArgument... Args>
    ImageView(ImageHandle texture, const Args&... args)
        : ImageView(Construction{ widgetType }, std::move(texture), std::tuple{ args... }) {
        endConstruction();
    }

protected:
    ImageHandle m_texture;

    void paint(Canvas& canvas) const override;
    Ptr cloneThis() override;

    ImageView(Construction construction, ImageHandle texture, ArgumentsView<ImageView> args);
};

class WIDGET SVGImageView final : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "svgimageview";

    template <WidgetArgument... Args>
    SVGImageView(SVGImage svg, const Args&... args)
        : Widget(Construction{ widgetType }, std::tuple{ args... }), m_svg(std::move(svg)) {
        endConstruction();
    }

    template <WidgetArgument... Args>
    SVGImageView(std::string_view svg, const Args&... args) : SVGImageView(SVGImage(svg), args...) {}

    ~SVGImageView();

protected:
    SVGImage m_svg;
    mutable RC<Image> m_image;

    void paint(Canvas& canvas) const override;
    Ptr cloneThis() override;

public:
    BRISK_PROPERTIES_BEGIN
    Property<SVGImageView, SVGImage, &SVGImageView::m_svg> svg;
    BRISK_PROPERTIES_END
};

} // namespace Brisk
