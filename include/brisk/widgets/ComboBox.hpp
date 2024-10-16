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

#include "ValueWidget.hpp"
#include "Item.hpp"
#include "ToggleButton.hpp"
#include "ItemList.hpp"
#include "Text.hpp"

namespace Brisk {

class WIDGET ComboBox : public ValueWidget {
public:
    using Base                                   = ValueWidget;
    constexpr static std::string_view widgetType = "combobox";

    template <WidgetArgument... Args>
    explicit ComboBox(const Args&... args) : ComboBox(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    template <typename T, typename... Args>
    explicit ComboBox(Value<T> prop,
                      std::shared_ptr<const NameValueOrderedList<std::type_identity_t<T>>> list,
                      const Args&... args)
        : ComboBox(Construction{ widgetType }, std::tuple{ args... }) {
        std::unique_ptr<ItemList> menu(new ItemList());
        for (const KeyValue<std::string, T>& item : *list) {
            menu->apply(new Item(new Text{ item.first }));
        }
        apply(menu.release());
        endConstruction();
        value = fromList(std::move(prop), list);
    }

    // roles
    std::shared_ptr<Item> selectedItem() const;
    std::shared_ptr<ToggleButton> unroll() const;
    std::shared_ptr<ItemList> menu() const;

protected:
    void onEvent(Event& event) override;

    template <typename T>
    Value<int> fromList(Value<T> value, std::shared_ptr<const NameValueOrderedList<T>> list) {
        return value.transform(
            [list](T value) -> int {
                return findValue(*list, value).value_or(-1);
            },
            [list](int index) -> T {
                if (index >= 0 && index < list->size())
                    return (*list)[index].second;
                return T{};
            });
    }

    void onChanged() override;
    std::shared_ptr<Item> findSelected() const;
    void onConstructed() override;
    void onChildAdded(Widget* w) override;
    Ptr cloneThis() override;
    explicit ComboBox(Construction construction, ArgumentsView<ComboBox> args);
};

} // namespace Brisk
