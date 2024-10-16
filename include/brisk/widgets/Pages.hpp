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

#include "Widgets.hpp"
#include "ToggleButton.hpp"

namespace Brisk {

class WIDGET TabButton : public ToggleButton {
public:
    using Base                                   = ToggleButton;
    constexpr static std::string_view widgetType = "tabbutton";

    template <WidgetArgument... Args>
    explicit TabButton(const Args&... args) : TabButton(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() override;

    explicit TabButton(Construction construction, ArgumentsView<TabButton> args);
};

class WIDGET Tabs : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "tabs";

    template <WidgetArgument... Args>
    explicit Tabs(const Args&... args) : Tabs{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

protected:
    Ptr cloneThis() override;
    explicit Tabs(Construction construction, ArgumentsView<Tabs> args);
};

class Pages;

class WIDGET Page : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "page";

    template <WidgetArgument... Args>
    explicit Page(std::string title, const Args&... args)
        : Page(Construction{ widgetType }, std::move(title), std::tuple{ args... }) {
        endConstruction();
    }

protected:
    std::string m_title;
    friend class Pages;
    Ptr cloneThis() override;

    explicit Page(Construction construction, std::string title, ArgumentsView<Page> args);
};

class WIDGET Pages : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "pages";

    enum {
        Horizontal = -1,
        Vertical   = -2,
    };

    template <WidgetArgument... Args>
    explicit Pages(Value<int> index, const Args&... args)
        : Pages(Construction{ widgetType }, std::move(index), std::tuple{ args... }) {
        endConstruction();
    }

    std::shared_ptr<Tabs> tabs() const;

    Value<int> index();

protected:
    int m_index = 0;
    void updateTabs();
    void childrenAdded() override;

    void onConstructed() override;

    virtual void onChanged();

    Ptr cloneThis() override;

    explicit Pages(Construction construction, Value<int> index, ArgumentsView<Pages> args);
};
} // namespace Brisk
