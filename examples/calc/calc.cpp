#include <brisk/core/internal/Initialization.hpp>
#include <brisk/gui/GUIApplication.hpp>
#include <brisk/gui/GUIWindow.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/graphics/Fonts.hpp>
#include "brisk/gui/Icons.hpp"
#include "math.hpp"

namespace Brisk {

struct CalcRow : public HLayout {
    template <WidgetArgument... Args>
    CalcRow(const Args&... args)
        : HLayout{
              args...,
              Arg::alignItems = AlignItems::Stretch,
              Arg::flexGrow   = 1,
              Arg::flexShrink = 0,
              Arg::flexBasis  = 0,
          } {}
};

struct CalcBtn : public Button {
    template <WidgetArgument... Args>
    CalcBtn(std::string text, const Args&... args)
        : Button{
              new Text{
                  std::move(text),
                  Arg::textAlign = TextAlign::Center,
              },
              Arg::borderRadius = 0,
              Arg::flexGrow     = 1,
              Arg::flexShrink   = 0,
              Arg::color        = 0xFDFDFD_rgb,
              Arg::flexBasis    = 0,
              Arg::fontSize     = 24,
              Arg::keyEvents    = ButtonKeyEvents::AcceptsSpace,
              args...,
          } {}
};

class CalcComponent final : public Component {
public:
    Calculator calc;

    void unhandledEvent(Event& event) final {
        if (event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::KPEnter)) {
            calc.solve();
            event.stopPropagation();
        }
        if (event.keyPressed(KeyCode::Escape)) {
            calc.clear();
            event.stopPropagation();
        }
        if (auto ch = event.characterTyped()) {
            switch (*ch) {
            case U'+':
                calc.operation(AdditiveOperator::Add);
                break;
            case U'-':
                calc.operation(AdditiveOperator::Subtract);
                break;
            case U'*':
                calc.operation(MultiplicativeOperator::Multiply);
                break;
            case U'/':
                calc.operation(MultiplicativeOperator::Divide);
                break;
            case U'.':
            case U',':
                calc.decimalSep();
                break;
            case U'=':
                calc.solve();
                break;
            case U'0':
            case U'1':
            case U'2':
            case U'3':
            case U'4':
            case U'5':
            case U'6':
            case U'7':
            case U'8':
            case U'9':
                calc.digit(*ch - U'0');
                break;
            default:
                return;
            }
            event.stopPropagation();
        }
    }

    explicit CalcComponent() {}

    RC<Widget> build() final {
        return rcnew VLayout{
            stylesheet = Graphene::stylesheet(),
            Graphene::darkColors(),
            minWidth              = 320_apx,
            windowColor           = 0x2E3747_rgb,
            animationSpeed        = 0.5,
            Graphene::buttonColor = 0x555B6E_rgb,
            alignItems            = AlignItems::Stretch,
            new Text{
                text              = calc.valOutput(),
                textAlign         = TextAlign::End,
                fontFamily        = Monospace,
                fontSize          = 40,
                padding           = 12,
                color             = 0x3F3F3F_rgb,
                backgroundColor   = 0xE4E4E4_rgb,
                height            = 1.5_em,
                textAutoSize      = TextAutoSize::FitWidth,
                textAutoSizeRange = { 12.f, 50.f },
            },
            new CalcRow{
                new CalcBtn{
                    "CE",
                    Graphene::buttonColor = 0x9A202A_rgb,
                    onClick               = m_lifetime |
                              [this] {
                                  calc.clear();
                              },
                },
                new CalcBtn{
                    "C",
                    onClick = m_lifetime |
                              [this] {
                                  calc.clear();
                              },
                },
                new CalcBtn{
                    ICON_pi, // "π",
                    onClick = m_lifetime |
                              [this] {
                                  calc.constant(Number::parse("3.1415926535897932384626433832795"));
                              },
                },
                new CalcBtn{
                    "←",
                    onClick = m_lifetime |
                              [this] {
                                  calc.backspace();
                              },
                },
            },
            new CalcRow{
                new CalcBtn{
                    "1/x",
                    onClick = m_lifetime |
                              [this] {
                                  calc.operation(UnaryOperator::Reciprocal);
                              },
                },
                new CalcBtn{
                    "x²",
                    onClick = m_lifetime |
                              [this] {
                                  calc.operation(UnaryOperator::Square);
                              },
                },
                new CalcBtn{
                    ICON_radical, // "√x",
                    onClick = m_lifetime |
                              [this] {
                                  calc.operation(UnaryOperator::SquareRoot);
                              },
                },
                new CalcBtn{
                    ICON_divide, // "÷",
                    Graphene::buttonColor = 0x6B7183_rgb,
                    onClick               = m_lifetime |
                              [this] {
                                  calc.operation(MultiplicativeOperator::Divide);
                              },
                },
            },
            new CalcRow{
                new CalcBtn{
                    "7",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(7);
                              },
                },
                new CalcBtn{
                    "8",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(8);
                              },
                },
                new CalcBtn{
                    "9",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(9);
                              },
                },
                new CalcBtn{
                    ICON_x, // "×",
                    Graphene::buttonColor = 0x6B7183_rgb,
                    onClick               = m_lifetime |
                              [this] {
                                  calc.operation(MultiplicativeOperator::Multiply);
                              },
                },
            },
            new CalcRow{
                new CalcBtn{
                    "4",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(4);
                              },
                },
                new CalcBtn{
                    "5",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(5);
                              },
                },
                new CalcBtn{
                    "6",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(6);
                              },
                },
                new CalcBtn{
                    ICON_minus, //  "−",
                    Graphene::buttonColor = 0x6B7183_rgb,
                    onClick               = m_lifetime |
                              [this] {
                                  calc.operation(AdditiveOperator::Subtract);
                              },
                },
            },
            new CalcRow{
                new CalcBtn{
                    "1",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(1);
                              },
                },
                new CalcBtn{
                    "2",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(2);
                              },
                },
                new CalcBtn{
                    "3",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(3);
                              },
                },
                new CalcBtn{
                    ICON_plus, //    "+",
                    Graphene::buttonColor = 0x6B7183_rgb,
                    onClick               = m_lifetime |
                              [this] {
                                  calc.operation(AdditiveOperator::Add);
                              },
                },
            },
            new CalcRow{
                new CalcBtn{
                    "±",
                    onClick = m_lifetime |
                              [this] {
                                  calc.changeSign();
                              },
                },
                new CalcBtn{
                    "0",
                    onClick = m_lifetime |
                              [this] {
                                  calc.digit(0);
                              },
                },
                new CalcBtn{
                    ".",
                    onClick = m_lifetime |
                              [this] {
                                  calc.decimalSep();
                              },
                },
                new CalcBtn{
                    ICON_equal, //  "=",
                    Graphene::buttonColor = 0x297227_rgb,
                    onClick               = m_lifetime |
                              [this] {
                                  calc.solve();
                              },
                },
            },
        };
    }

    void configureWindow(RC<GUIWindow> window) final {
        window->setTitle("Calc"_tr);
        window->setSize({ 742, 525 });
        window->windowFit = WindowFit::MinimumSize;
        window->setStyle(WindowStyle::Normal);
    }
};

} // namespace Brisk

using namespace Brisk;

int briskMain() {
    GUIApplication application;

    return application.run(createComponent<CalcComponent>());
}
