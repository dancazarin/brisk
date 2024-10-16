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
#include <brisk/widgets/SpinBox.hpp>
#include <brisk/gui/Icons.hpp>

namespace Brisk {

std::shared_ptr<Text> SpinBox::text() const {
    return find<Text>(MatchAny{});
}

std::shared_ptr<UpDownButtons> SpinBox::buttons() const {
    return find<UpDownButtons>(MatchAny{});
}

void SpinBox::onChildAdded(Widget* w) {
    ValueWidget::onChildAdded(w);
    if (Text* text = dynamic_cast<Text*>(w)) {
        text->role     = "display";
        text->flexGrow = 1;
        text->text     = Value{ &this->value }.transform([this](double value) -> std::string {
            return m_valueFormatter(value);
        });
    }
    if (UpDownButtons* btns = dynamic_cast<UpDownButtons*>(w)) {
        btns->role     = "btns";
        btns->flexGrow = 0;

        ArgumentsView<Button>{
            std::tuple{ Arg::onClick = listener(
                            [this]() {
                                increment();
                            },
                            this) }
        }.apply(btns->btnUp().get());
        ArgumentsView<Button>{
            std::tuple{ Arg::onClick = listener(
                            [this]() {
                                decrement();
                            },
                            this) }
        }.apply(btns->btnDn().get());
    }
}

void SpinBox::onConstructed() {
    m_layout = Layout::Horizontal;

    if (auto text = this->text()) {
    } else {
        apply(new Text{ "", Arg::role = "display" });
    }
    if (auto buttons = this->buttons()) {
    } else {
        apply(new UpDownButtons{ Arg::role = "btns" });
    }
    ValueWidget::onConstructed();
}

void SpinBox::onEvent(Event& event) {
    ValueWidget::onEvent(event);
    if (float delta = event.wheelScrolled(m_rect, m_wheelModifiers)) {
        value = m_value + delta;
        event.stopPropagation();
        return;
    }
    if (event.keyPressed(KeyCode::Up)) {
        increment();
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Down)) {
        decrement();
        event.stopPropagation();
    }
}

SpinBox::SpinBox(Construction construction, ArgumentsView<SpinBox> args)
    : ValueWidget(construction, nullptr) {
    layout  = Layout::Horizontal;
    tabStop = true;
    args.apply(this);
}

Widget::Ptr SpinBox::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

UpDownButtons::UpDownButtons(Construction construction, ArgumentsView<UpDownButtons> args)
    : Widget(construction, nullptr) {
    layout = Layout::Vertical;
    args.apply(this);
}

std::shared_ptr<Button> UpDownButtons::btnDn() const {
    return find<Button>(MatchNth{ 1 });
}

std::shared_ptr<Button> UpDownButtons::btnUp() const {
    return find<Button>(MatchNth{ 0 });
}

void UpDownButtons::onChildAdded(Widget* w) {
    Widget::onChildAdded(w);

    if (Button* btn = dynamic_cast<Button*>(w)) {
        if (!btnUp()) {
            btn->role = "up";
        } else {
            btn->role = "down";
        }
        btn->tabStop        = false;
        btn->flexGrow       = 1;
        btn->clickEvent     = ButtonClickEvent::MouseDown;
        btn->repeatDelay    = 0.5;
        btn->repeatInterval = 0.25;
    }
}

void UpDownButtons::onConstructed() {
    if (auto text = this->btnUp()) {
    } else {
        apply(new Button{ new Text{ ICON_chevron_up }, Arg::role = "up" });
    }
    if (auto buttons = this->btnDn()) {
    } else {
        apply(new Button{ new Text{ ICON_chevron_down }, Arg::role = "down" });
    }
    Widget::onConstructed();
}

Widget::Ptr UpDownButtons::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}
} // namespace Brisk
