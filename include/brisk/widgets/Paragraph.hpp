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
#include <brisk/core/Utilities.hpp>

namespace Brisk {

class Paragraph final : public Widget {
public:
    template <WidgetArgument... Args>
    explicit Paragraph(std::string text, const Args&... args)
        : Widget{ Construction{ "paragraph" }, std::tuple{ args... } }, m_text(std::move(text)) {
        enableCustomMeasure();
        endConstruction();
    }

    optional<std::string> textContent() const override;

    using Widget::apply;

protected:
    void onLayoutUpdated() override;
    void paint(Canvas& canvas) const override;
    Size prelayout(int width) const;
    std::string m_text;

    struct ParagraphCache {
        std::u32string text;
        ShapedRuns shaped;
    };

    struct ParagraphCache2 {
        PrerenderedText prerendered;
    };

    mutable Cache<ParagraphCache, Font, std::string> m_cache;
    mutable Cache<ParagraphCache2, int, TextAlign> m_cache2;
    ParagraphCache& updateCache() const;
    ParagraphCache2& updateCache2(int width) const;
    SizeF measure(AvailableSize size) const override;
};
} // namespace Brisk
