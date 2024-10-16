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
#include <brisk/widgets/Paragraph.hpp>
#include <brisk/core/Text.hpp>

namespace Brisk {

Paragraph::ParagraphCache& Paragraph::updateCache() const {
    return m_cache(
        [](const Font& font, const std::string& m_text) mutable -> ParagraphCache {
            std::u32string text32 = utf8ToUtf32(m_text);
            ShapedRuns shaped     = fonts->shape(font, text32);
            return ParagraphCache{ std::move(text32), std::move(shaped) };
        },
        font(), m_text);
}

Paragraph::ParagraphCache2& Paragraph::updateCache2(int width) const {
    ParagraphCache& cache = updateCache();
    Font font             = this->font();
    return m_cache2(
        [&cache, &font](int width, TextAlign x_align) mutable -> ParagraphCache2 {
            const RectangleF rect{ 0, 0, float(width), float(INT_MAX) };
            PrerenderedText prerendered = cache.shaped.prerender(font, width);
            prerendered.alignLines(rect, toFloatAlign(x_align), 0.f);
            return ParagraphCache2{ std::move(prerendered) };
        },
        width, m_textAlign);
}

SizeF Paragraph::measure(AvailableSize size) const {
    int width   = size.x.valueOr(HUGE_VALF);
    Size result = prelayout(std::max(0, width));
    return result;
}

Size Paragraph::prelayout(int width) const {
    ParagraphCache& cache = updateCache();
    RectangleF rect       = cache.shaped.prerender(font(), width).bounds();
    rect.x1               = std::floor(rect.x1);
    rect.y1               = std::floor(rect.y1);
    rect.x2               = std::ceil(rect.x2);
    rect.y2               = std::ceil(rect.y2);
    return rect.size();
}

void Paragraph::onLayoutUpdated() {}

void Paragraph::paint(Canvas& canvas) const {
    Widget::paint(canvas);
    const Rectangle rect        = m_clientRect;
    ParagraphCache2& cache2     = updateCache2(rect.width());
    PrerenderedText prerendered = cache2.prerendered;
    RectangleF bounds           = prerendered.bounds();
    prerendered.applyOffset(
        PointF(rect.x1,
               rect.y1 - (bounds.height() - rect.height()) * toFloatAlign(m_textVerticalAlign) - bounds.y1));
    canvas.raw().drawText(prerendered, fillColor = m_color.current);
}

optional<std::string> Paragraph::textContent() const {
    return m_text;
}
} // namespace Brisk
