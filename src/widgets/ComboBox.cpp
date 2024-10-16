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
#include <brisk/widgets/ComboBox.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/gui/Icons.hpp>
#include <brisk/gui/Styles.hpp>

namespace Brisk {

std::shared_ptr<Item> ComboBox::selectedItem() const {
    return find<Item>(MatchAny{});
}

std::shared_ptr<ToggleButton> ComboBox::unroll() const {
    return find<ToggleButton>(MatchAny{});
}

std::shared_ptr<ItemList> ComboBox::menu() const {
    return find<ItemList>(MatchAny{});
}

void ComboBox::onChildAdded(Widget* w) {
    Base::onChildAdded(w);
    if (ItemList* menu = dynamic_cast<ItemList*>(w)) {
        menu->role        = "itemlist";
        menu->onItemClick = [this](size_t index) BRISK_INLINE_LAMBDA {
            value = index;
        };
        menu->onBecameVisible = [this]() BRISK_INLINE_LAMBDA {
            if (auto item = findSelected()) {
                item->focus();
            }
        };
        menu->visible          = false;
        menu->absolutePosition = { 0, 100_perc };
        menu->anchor           = { 0, 0 };
        menu->tabGroup         = true;
    }
    if (Item* selectedItem = dynamic_cast<Item*>(w)) {
        selectedItem->role             = "selecteditem";
        selectedItem->flexGrow         = 1;
        selectedItem->tabStop          = false;
        selectedItem->mouseInteraction = MouseInteraction::Disable;
    }
    if (ToggleButton* unroll = dynamic_cast<ToggleButton*>(w)) {
        unroll->role             = "unroll";
        unroll->tabStop          = false;
        unroll->mouseInteraction = MouseInteraction::Disable;
        unroll->borderWidth      = 0;
        unroll->flexGrow         = 0;
    }
}

void ComboBox::onConstructed() {
    m_layout = Layout::Horizontal;

    if (auto menu = this->menu()) {
    } else {
        apply(new ItemList{});
    }
    if (auto selectedItem = this->selectedItem()) {
    } else {
        apply(new Item{ Arg::role = "selecteditem" });
    }
    if (auto unroll = this->unroll()) {
    } else {
        apply(new ToggleButton{ Arg::value = Value{ &menu()->visible }, new Text{ ICON_chevron_down },
                                new Text{ ICON_chevron_up }, Arg::twoState = true });
    }
    Base::onConstructed();
}

std::shared_ptr<Item> ComboBox::findSelected() const {
    if (!m_constructed)
        return nullptr;
    auto menu     = this->menu();
    auto& widgets = menu->widgets();
    int value     = std::round(m_value);
    if (value < 0 || value >= widgets.size())
        return nullptr;
    return std::dynamic_pointer_cast<Item>(widgets[value]);
}

void ComboBox::onEvent(Event& event) {
    auto menu = this->menu();
    Base::onEvent(event);
    if (float delta = event.wheelScrolled(m_rect, m_wheelModifiers)) {
        int val = std::clamp(int(m_value - delta), 0, int(menu->widgets().size() - 1));
        value   = val;
        event.stopPropagation();
    } else if (event.pressed()) {
        focus();
        auto passedThroughBy = inputQueue->passedThroughBy.lock();
        if (passedThroughBy != menu)
            menu->visible = true;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        menu->visible = !menu->visible;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Escape)) {
        menu->visible = false;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Up)) {
        int val = std::clamp(int(m_value - 1), 0, int(menu->widgets().size() - 1));
        value   = val;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Down)) {
        int val = std::clamp(int(m_value + 1), 0, int(menu->widgets().size() - 1));
        value   = val;
        event.stopPropagation();
    }
}

void ComboBox::onChanged() {
    if (!m_constructed)
        return;
    auto w = findSelected();
    std::shared_ptr<Item> cloned;
    if (w) {
        cloned = std::dynamic_pointer_cast<Item>(w->clone());
    } else {
        cloned = std::shared_ptr<Item>(new Item{});
    }
    cloned->role     = "selecteditem";
    cloned->flexGrow = 1;
    cloned->tabStop  = false;
    bool replaced    = replace(selectedItem(), cloned, false);
    BRISK_ASSERT(replaced);
}

Widget::Ptr ComboBox::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

ComboBox::ComboBox(Construction construction, ArgumentsView<ComboBox> args) : Base(construction, nullptr) {
    m_tabStop       = true;
    m_processClicks = false;
    args.apply(this);
}
} // namespace Brisk
