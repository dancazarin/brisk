#include "Typography.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/window/Clipboard.hpp>

namespace Brisk {

static Builder iconsBuilder() {
    return Builder([](Widget* target) {
        constexpr int columns = 16;
        auto iconFontFamily   = GoNoto;
        int iconFontSize      = 25;
        for (int icon = ICON__first; icon < ICON__last; icon += columns) {
            HLayout* glyphs = new HLayout{
                new Text{
                    fmt::format("{:04X}", icon),
                    textVerticalAlign = TextAlign::Center,
                    dimensions        = { 60, 50 },
                },
            };
            for (int c = 0; c < columns; c++) {
                char32_t ch = icon + c;
                string u8   = utf32ToUtf8(std::u32string(1, ch));
                glyphs->apply(new Text{
                    u8,
                    classes           = { "icon" },
                    textAlign         = TextAlign::Center,
                    textVerticalAlign = TextAlign::Center,
                    fontFamily        = iconFontFamily,
                    fontSize          = iconFontSize,
                    dimensions        = { 50, 50 },
                    onClick           = listener(
                        [ch] {
                            copyTextToClipboard(fmt::format("\\u{:04X}", uint32_t(ch)));
                        },
                        &staticBinding),
                });
            }
            target->apply(glyphs);
        }
    });
}

const std::string pangram = "The quick brown fox jumps over the lazy dog 0123456789";

RC<Widget> ShowcaseTypography::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        new Text{ "Fonts", classes = { "section-header" } },

        new HScrollBox{
            new VLayout{
                flexGrow = 1,
                Builder([](Widget* target) {
                    for (int i = 0; i < 7; ++i) {
                        int size = 8 + i * 4;
                        auto row = [target, size](std::string name, FontFamily family, FontWeight weight) {
                            target->apply(new Text{
                                pangram + fmt::format(" [{}, {}px]", name, size),
                                fontFamily = family,
                                fontWeight = weight,
                                fontSize   = size,
                            });
                        };
                        row("Lato Light", Lato, FontWeight::Light);
                        row("Lato Regular", Lato, FontWeight::Regular);
                        row("Lato Bold", Lato, FontWeight::Bold);
                        row("GoNoto", GoNoto, FontWeight::Regular);
                        row("Monospace", Monospace, FontWeight::Regular);
                        target->apply(new Spacer{ height = 12_apx });
                    }
                }),
            },
        },

        new Text{ "Icons (gui/Icons.hpp)", classes = { "section-header" } },

        new VLayout{
            padding = { 8_apx, 8_apx },

            iconsBuilder(),
        },
    };
}
} // namespace Brisk
