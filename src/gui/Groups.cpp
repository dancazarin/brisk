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
#include <brisk/gui/Groups.hpp>

namespace Brisk {

void SizeGroup::beforeLayout(bool dirty) {
    if (!dirty)
        return;
    SizeF m{ 0, 0 };
    bool isDirty = false;
    for (Widget* w : widgets) {
        if (w->isLayoutDirty()) [[likely]] {
            isDirty = true;
            break;
        }
    }
    if (!isDirty) [[likely]]
        return;
    for (Widget* w : widgets) {
        m = max(m, w->computeSize(AvailableSize{ undef, undef }));
    }

    float flexBasis = m[+orientation];
    BRISK_ASSERT(!std::isnan(flexBasis));
    for (Widget* w : widgets) {
        w->flexBasis.set(flexBasis * 1_dpx);
    }
}

void VisualGroup::beforeFrame() {
    for (size_t i = 0; i < widgets.size(); ++i) {
        Widget* w = widgets[i];
        int c     = 0b0000;
        if (i == 0) {
            // the first one
            c |= orientation == Orientation::Horizontal ? +CornerFlags::Left : +CornerFlags::Top;
        } else {
            // not the first one
            w->marginLeft      = 0;
            w->borderWidthLeft = 0;
        }
        if (i == widgets.size() - 1) {
            // the last one
            c |= orientation == Orientation::Horizontal ? +CornerFlags::Right : +CornerFlags::Bottom;
        } else {
            // not the last one
            w->marginRight      = 0;
            w->borderWidthRight = 0;
        }

        w->corners = c;
    }
}
} // namespace Brisk
