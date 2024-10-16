#pragma once

#include <brisk/widgets/Notifications.hpp>

namespace Brisk {

class ShowcaseTypography : public BindingObject<ShowcaseTypography, &uiThread> {
public:
    RC<Widget> build(RC<Notifications> notifications);

private:
};
} // namespace Brisk
