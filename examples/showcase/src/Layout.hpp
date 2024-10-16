#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseLayout : public BindingObject<ShowcaseLayout, &uiThread> {
public:
    RC<Widget> build(RC<Notifications> notifications);

private:
    WidthGroup m_group;
};
} // namespace Brisk
