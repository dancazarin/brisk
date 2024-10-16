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
#include <brisk/widgets/Pages.hpp>

namespace Brisk {

void Pages::updateTabs() {
    std::shared_ptr<Tabs> tabs = this->tabs();
    if (!tabs)
        return;
    tabs->clear();
    int index = 0;
    for (Widget::Ptr w : *this) {
        if (Page* p = dynamic_cast<Page*>(w.get())) {
            auto prop = this->index() == index;
            tabs->apply(new TabButton(Arg::value = std::move(prop), new Text{ p->m_title }));
            ++index;
        }
    }
    onChanged();
}

std::shared_ptr<Tabs> Pages::tabs() const {
    auto tabs = this->find<Tabs>(MatchAny{});
    return tabs;
}

void Pages::childrenAdded() {
    Widget::childrenAdded();
    updateTabs();
}

Value<int> Pages::index() {
    return Value<int>{
        &m_index,
        [this]() {
            onChanged();
        },
    };
}

void Pages::onChanged() {
    if (m_index == Horizontal) {
        layout = Layout::Horizontal;
    } else if (m_index == Vertical) {
        layout = Layout::Vertical;
    }
    int index = 0;
    for (Widget::Ptr w : *this) {
        if (Page* p = dynamic_cast<Page*>(w.get())) {
            p->visible = m_index < 0 || m_index == index;
            ++index;
        }
    }
}

TabButton::TabButton(Construction construction, ArgumentsView<TabButton> args) : Base(construction, nullptr) {
    args.apply(this);
}

Widget::Ptr TabButton::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Widget::Ptr Pages::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Widget::Ptr Page::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Widget::Ptr Tabs::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

Tabs::Tabs(Construction construction, ArgumentsView<Tabs> args)
    : Widget{ construction, std::tuple{ Arg::tabGroup = true } } {
    args.apply(this);
}

Page::Page(Construction construction, std::string title, ArgumentsView<Page> args)
    : Widget(construction, std::tuple{ Arg::layout = Layout::Horizontal, Arg::flexGrow = true,
                                       Arg::alignItems = AlignItems::Stretch }),
      m_title(std::move(title)) {
    args.apply(this);
}

Pages::Pages(Construction construction, Value<int> index, ArgumentsView<Pages> args)
    : Widget(construction,
             std::tuple{ Arg::layout = Layout::Vertical, Arg::alignItems = AlignItems::Stretch }) {
    args.apply(this);
    bindings->connectBidir(this->index(), std::move(index));
    updateTabs();
}

void Pages::onConstructed() {
    Widget::onConstructed();
    if (auto tabs = this->tabs()) {
    } else {
        apply(new Tabs{});
    }
}
} // namespace Brisk
