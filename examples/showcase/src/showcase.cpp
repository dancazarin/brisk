#include "ShowcaseComponent.hpp"
#include <brisk/gui/GUIApplication.hpp>

int briskMain() {
    using namespace Brisk;

    GUIApplication application;
    return application.run(createComponent<ShowcaseComponent>());
}
