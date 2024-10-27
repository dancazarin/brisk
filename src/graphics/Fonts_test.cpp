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
#include <fmt/ranges.h>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/core/Utilities.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include "../core/test/HelloWorld.hpp"
#include <brisk/core/Reflection.hpp>
#include "VisualTests.hpp"
#include "Atlas.hpp"

namespace Brisk {

static std::string_view fontFamiles[2] = {
    "Lato",
    "Noto",
};
std::string glyphRunToString(const GlyphRun& run);
} // namespace Brisk

template <>
struct fmt::formatter<Brisk::FontMetrics> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::FontMetrics& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{{ {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f} }}", value.size,
                        value.ascender, value.descender, value.height, value.spaceAdvanceX,
                        value.lineThickness, value.xHeight, value.capitalHeight),
            ctx);
    }
};

template <>
struct fmt::formatter<Brisk::FontFamily> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::FontFamily& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(fmt::format("{};", Brisk::fontFamiles[+value]), ctx);
    }
};

template <>
struct fmt::formatter<Brisk::FontManager::FontKey> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::FontManager::FontKey& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{} {} {}", std::get<0>(value), std::get<1>(value), std::get<2>(value)), ctx);
    }
};

template <>
struct fmt::formatter<Brisk::GlyphRun> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::GlyphRun& value, FormatContext& ctx) const {

        return fmt::formatter<std::string>::format(Brisk::glyphRunToString(value), ctx);
    }
};

template <>
struct fmt::formatter<Brisk::PrerenderedText> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::PrerenderedText& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("Prerendered\n{}\n///////////////////////", fmt::join(value.runs, "\n----\n")), ctx);
    }
};

namespace Brisk {
static RC<FontManager> fontManager;

static std::string codepointDetails(char32_t value) {
    if (uint32_t(value) >= 32)
        return fmt::format("{} ('{}')", unicodeChar(value),
                           Brisk::utf32ToUtf8(std::u32string_view(&value, 1)));
    else
        return fmt::format("{}      ", unicodeChar(value));
}

std::string glyphRunToString(const GlyphRun& run) {
    std::string result =
        fmt::format("Positioned at {{ {:6.2f}, {:6.2f} }}:\n", run.position.x, run.position.y);
    for (int i = 0; i < run.glyphs.size(); ++i) {
        const Internal::Glyph& g        = run.glyphs[i];
        Internal::GlyphData d           = g.load(run).value_or(Internal::GlyphData{});
        Brisk::FontManager::FontKey key = Brisk::fontManager->faceToKey(run.face);

        std::string flags;
        if (g.flags && Internal::GlyphFlags::AtLineBreak)
            flags += "ALB ";
        else
            flags += "    ";
        if (g.flags && Internal::GlyphFlags::SafeToBreak)
            flags += "STB ";
        else
            flags += "    ";

        result += fmt::format(
            "{{ {:5}, {}, cl={:2}..{:<2}, L={:6.2f}, R={:6.2f}, w={:6.2f}, pos={{ {:6.2f}, {:6.2f} }}, "
            "adv={:6.2f}, sz={:2d}x{:<2d}, sp={:08X}, {}, {} }}\n",
            g.glyph, codepointDetails(g.codepoint), g.begin_char, g.end_char, g.left_caret, g.right_caret,
            g.right_caret - g.left_caret, g.pos.x, g.pos.y, d.advance_x, d.size.x, d.size.y, UINT32_MAX, key,
            flags);
    }
    return result;
}

} // namespace Brisk

namespace Brisk {

TEST_CASE("textBreakPositions") {
    CHECK(utf32ToUtf16(U"abc").size() == 3);
    CHECK(utf32ToUtf16(U"αβγ").size() == 3);
    CHECK(utf32ToUtf16(U"一二三").size() == 3);
    CHECK(utf32ToUtf16(U"𠀀𠀁𠀂").size() == 6);
    CHECK(textBreakPositions(U"abc", TextBreakMode::Grapheme) == std::vector<int32_t>{ 0, 1, 2, 3 });
    CHECK(textBreakPositions(U"αβγ", TextBreakMode::Grapheme) == std::vector<int32_t>{ 0, 1, 2, 3 });
    CHECK(textBreakPositions(U"一二三", TextBreakMode::Grapheme) == std::vector<int32_t>{ 0, 1, 2, 3 });
    CHECK(textBreakPositions(U"𠀀𠀁𠀂", TextBreakMode::Grapheme) == std::vector<int32_t>{ 0, 1, 2, 3 });

    CHECK(textBreakPositions(U"abc abc", TextBreakMode::Grapheme) ==
          std::vector<int32_t>{ 0, 1, 2, 3, 4, 5, 6, 7 });
    CHECK(textBreakPositions(U"𠀀𠀁𠀂 𠀀𠀁𠀂", TextBreakMode::Grapheme) ==
          std::vector<int32_t>{ 0, 1, 2, 3, 4, 5, 6, 7 });

    CHECK(textBreakPositions(U"á", TextBreakMode::Grapheme) == std::vector<int32_t>{ 0, 2 });
    CHECK(textBreakPositions(U"á̈", TextBreakMode::Grapheme) == std::vector<int32_t>{ 0, 3 });

    CHECK(textBreakPositions(U"abc", TextBreakMode::Word) == std::vector<int32_t>{ 0, 3 });
    CHECK(textBreakPositions(U"abc def", TextBreakMode::Word) == std::vector<int32_t>{ 0, 3, 4, 7 });

    CHECK(textBreakPositions(U"abc", TextBreakMode::Line) == std::vector<int32_t>{ 0, 3 });
    CHECK(textBreakPositions(U"abc def", TextBreakMode::Line) == std::vector<int32_t>{ 0, 4, 7 });

    CHECK(textBreakPositions(U"A B C D E F", TextBreakMode::Line) ==
          std::vector<int32_t>{ 0, 2, 4, 6, 8, 10, 11 });
}

TEST_CASE("splitTextRuns") {
    CHECK(Internal::splitTextRuns(U"𠀀𠀁𠀂 𠀀𠀁𠀂", TextDirection::LTR, true) ==
          std::vector<Internal::TextRun>{
              Internal::TextRun{
                  .direction = TextDirection::LTR, .begin = 0, .end = 7, .visualOrder = 0, .face = nullptr },
          });
    if (icuAvailable) {
        CHECK(Internal::splitTextRuns(U"𠀀𠀁𠀂 \U0000200F123\U0000200E 𠀀𠀁𠀂", TextDirection::LTR, true) ==
              std::vector<Internal::TextRun>{
                  Internal::TextRun{ .direction   = TextDirection::LTR,
                                     .begin       = 0,
                                     .end         = 4,
                                     .visualOrder = 0,
                                     .face        = nullptr },
                  Internal::TextRun{ .direction   = TextDirection::LTR,
                                     .begin       = 5,
                                     .end         = 8,
                                     .visualOrder = 7,
                                     .face        = nullptr },
                  Internal::TextRun{ .direction   = TextDirection::RTL,
                                     .begin       = 4,
                                     .end         = 5,
                                     .visualOrder = 10,
                                     .face        = nullptr },
                  Internal::TextRun{ .direction   = TextDirection::LTR,
                                     .begin       = 8,
                                     .end         = 13,
                                     .visualOrder = 11,
                                     .face        = nullptr },
              });
    }
}

TEST_CASE("FontManager") {

    fontManager = rcnew FontManager(nullptr, 1, 5000);

    auto ttf    = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(ttf.has_value());
    auto ttf2 = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "GoNotoCurrent-Regular.ttf");
    REQUIRE(ttf2.has_value());

    FontFamily lato = FontFamily(0);
    FontFamily noto = FontFamily(1);

    fontManager->addFont(lato, FontStyle::Normal, FontWeight::Regular, *ttf, true, FontFlags::Default);
    CHECK(fontManager->fontFamilyStyles(lato) == std::vector<FontStyleAndWeight>{ FontStyleAndWeight{
                                                     FontStyle::Normal,
                                                     FontWeight::Regular,
                                                 } });

    fontManager->addFont(noto, FontStyle::Normal, FontWeight::Regular, *ttf2, true, FontFlags::Default);
    FontFamily latoPlusNoto = FontFamily(2);
    fontManager->addMergedFont(latoPlusNoto, { lato, noto });

    Font font;
    font.fontFamily     = latoPlusNoto;
    font.lineHeight     = 1.f;
    FontMetrics metrics = fontManager->metrics(font);
    CHECK(metrics == FontMetrics{ 10, 10, -3, 12, 2.531250, 0.750, 5.080, 7.180 });

    PrerenderedText run;
    run = fontManager->prerender(font, U"Hello, world!"s);
    fmt::print("{}\n\n", run);

    run = fontManager->prerender(font, U"world!"s);
    fmt::print("{}\n\n", run);

    run = fontManager->prerender(font, U"A B C D E F"s);
    fmt::print("{}\n\n", run);

    run = fontManager->prerender(font, U"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z"s);
    fmt::print("{}\n\n", run);

    run = fontManager->prerender(font, utf8ToUtf32(helloWorld[7]));
    fmt::print("{}\n\n", run);

    run = fontManager->prerender(font, utf8ToUtf32(helloWorld[11]));
    fmt::print("{}\n\n", run);

    run = fontManager->prerender(font, utf8ToUtf32(helloWorld[53]));
    fmt::print("{}\n\n", run);

    run = fontManager->prerender(font, utf8ToUtf32(helloWorld[57]));
    fmt::print("{}\n\n", run);

    fmt::println("breaks: {}",
                 fmt::join(textBreakPositions(utf8ToUtf32(helloWorld[57]), TextBreakMode::Line), ", "));

    RectangleF bounds          = fontManager->bounds(font, U"Hello, world!"s);
    bounds                     = fontManager->bounds(font, U"  Hello, world!"s);
    bounds                     = fontManager->bounds(font, U"  Hello, world!  "s);
    bounds                     = fontManager->bounds(font, U"Hello, world!  "s);

    [[maybe_unused]] Size size = { 512, 64 };

    Font bigFont{ latoPlusNoto, 36.f };

    static std::set<int> requiresIcu{
        3, 25, 47, 48, 71,
    };
    for (int i = 0; i < std::size(helloWorld); i++) {
        if (!icuAvailable && requiresIcu.contains(i)) {
            continue;
        }
        visualTestMono(fmt::format("hello{}", i), { 512, 64 }, [&](RC<Image> image) {
            Font font{ latoPlusNoto, 36.f };
            auto run = fontManager->prerender(font, helloWorld[i]);
            fontManager->testRender(image, run, { 5, 42 });
        });
    }

    visualTestMono("nl-multi", { 256, 128 }, [&](RC<Image> image) {
        auto run = fontManager->prerender(bigFont,
                                          TextWithOptions{ utf8ToUtf32("ABC\nDEF"), LayoutOptions::Default });
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("nl-single", { 256, 128 }, [&](RC<Image> image) {
        auto run = fontManager->prerender(
            bigFont, TextWithOptions{ utf8ToUtf32("ABC\nDEF"), LayoutOptions::SingleLine });
        fontManager->testRender(image, run, { 5, 42 });
    });

    constexpr auto diac =
        UR"(a	à	â	ă	å	a̋	ä	a̧	ǎ	ã	á	ä́	á̈
i	ì	î	ĭ	i̊	i̋	ï	i̧	ǐ	ĩ	í	ḯ	í̈
q	q̀	q̂	q̆	q̊	q̋	q̈	q̧	q̌	q̃	q́	q̈́	q́̈
I	Ì	Î	Ĭ	I̊	I̋	Ï	I̧	Ǐ	Ĩ	Í	Ḯ	Í̈
Њ	Њ̀	Њ̂	Њ̆	Њ̊	Њ̋	Њ̈	Њ̧	Њ̌	Њ̃	Њ́	Њ̈́	Њ́̈
)";

    visualTestMono("diacritics-lato", { 650, 290 }, [&](RC<Image> image) {
        Font font{ lato, 36.f };
        font.tabWidth = 5.f;
        auto run      = fontManager->prerender(font, diac);
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("diacritics-noto", { 650, 290 }, [&](RC<Image> image) {
        Font font{ noto, 36.f };
        font.tabWidth = 5.f;
        auto run      = fontManager->prerender(font, diac);
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("bounds-text", { 128, 64 }, [&](RC<Image> image) {
        Font font{ lato, 36.f };
        auto run = fontManager->prerender(font, U"a");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text-bar", { 128, 64 }, [&](RC<Image> image) {
        Font font{ lato, 36.f };
        auto run = fontManager->prerender(font, U"|");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text2", { 128, 64 }, [&](RC<Image> image) {
        Font font{ lato, 36.f };
        auto run = fontManager->prerender(font, U"a  ");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text3", { 128, 64 }, [&](RC<Image> image) {
        Font font{ lato, 36.f };
        auto run = fontManager->prerender(font, U"  a");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });

    visualTestMono("lineHeight1", { 64, 64 }, [&](RC<Image> image) {
        Font font{ lato, 16.f };
        font.lineHeight = 1.f;
        auto run        = fontManager->prerender(font, U"1st line\n2nd line");
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("lineHeight1dot5", { 64, 64 }, [&](RC<Image> image) {
        Font font{ lato, 16.f };
        font.lineHeight = 1.5f;
        auto run        = fontManager->prerender(font, U"1st line\n2nd line");
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::TextBounds);
    });

    visualTestMono("unicode-suppl", { 256, 64 }, [&](RC<Image> image) {
        Font font{ noto, 36.f };
        auto run = fontManager->prerender(
            font, U"\U00010140\U00010141\U00010142\U00010143\U00010144\U00010145\U00010146\U00010147");
        fontManager->testRender(image, run, { 5, 42 });
    });

    if (icuAvailable) {
        visualTestMono("mixed", { 512, 64 }, [&](RC<Image> image) {
            Font font{ noto, 36.f };
            auto run = fontManager->prerender(font, U"abcdef مرحبا بالعالم!");
            fontManager->testRender(image, run, { 5, 42 });
        });

        visualTestMono("mixed2", { 512, 64 }, [&](RC<Image> image) {
            Font font{ noto, 36.f };
            auto run = fontManager->prerender(font, U"123456 مرحبا بالعالم!");
            fontManager->testRender(image, run, { 5, 42 });
        });

        visualTestMono("mixed3", { 512, 64 }, [&](RC<Image> image) {
            Font font{ noto, 36.f };
            auto run = fontManager->prerender(font, U"مرحبا (بالعالم)!");
            fontManager->testRender(image, run, { 5, 42 });
        });
    }

    visualTestMono("wrapped-abc", { 128, 128 }, [&](RC<Image> image) {
        Font font{ noto, 18.f };
        auto t      = U"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
        auto shaped = fontManager->shape(font, t);
        auto run    = shaped.prerender(font, 120);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 120 }, { 20 });
    });
    visualTestMono("wrapped-abc-big", { 4 * 128, 4 * 128 }, [&](RC<Image> image) {
        Font font{ noto, 4 * 18.f };
        auto t      = U"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
        auto shaped = fontManager->shape(font, t);
        auto run    = shaped.prerender(font, 4 * 120);
        fontManager->testRender(image, run, { 4 * 3, 4 * 20 }, TestRenderFlags::None,
                                { 4 * 3, 4 * 3 + 4 * 120 }, { 4 * 20 });
    });
    visualTestMono("wrapped-abc2", { 128, 128 }, [&](RC<Image> image) {
        Font font{ noto, 18.f };
        auto t      = U"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
        auto shaped = fontManager->shape(font, t);
        auto run    = shaped.prerender(font, 110);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 110 }, { 20 });
    });
    visualTestMono("wrapped-abc3", { 128, 128 }, [&](RC<Image> image) {
        Font font{ noto, 16.f };
        auto t      = U"ABCDEFGHIJKLM N O P Q R S T U V W X Y Z";
        auto shaped = fontManager->shape(font, t);
        auto run    = shaped.prerender(font, 100);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 100 }, { 20 });
    });
    visualTestMono("wrapped-abc4", { 128, 128 }, [&](RC<Image> image) {
        Font font{ noto, 16.f };
        auto t      = U"A               B C D E F G H";
        auto shaped = fontManager->shape(font, t);
        auto run    = shaped.prerender(font, 24);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 24 }, { 20 });
    });
    if (icuAvailable) {
        visualTestMono("wrapped-rtl4", { 128, 128 }, [&](RC<Image> image) {
            Font font{ noto, 16.f };
            auto t      = U"א          ב ג ד ה ו ז ח ט י";
            auto shaped = fontManager->shape(font, t);
            auto run    = shaped.prerender(font, 24);
            run.alignLines(PointF{ 24, 0 }, 1.f, 0.f);
            fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 24 }, { 20 });
        });
    }
    visualTestMono("wrapped-abc0", { 128, 128 }, [&](RC<Image> image) {
        Font font{ noto, 16.f };
        auto t      = U"ABCDEFGHIJKLM N O P Q R S T U V W X Y Z";
        auto shaped = fontManager->shape(font, t);
        auto run    = shaped.prerender(font, 0);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3 }, { 20 });
    });
    if (icuAvailable) {
        visualTestMono("wrapped-cn", { 490, 384 }, [&](RC<Image> image) {
            Font font{ noto, 32.f };
            auto t =
                U"人人生而自由，在尊严和权利上一律平等。他们赋有理性和良心，并应以兄弟关系的精神相对待。";
            auto shaped = fontManager->shape(font, t);
            auto run    = shaped.prerender(font, 460);
            fontManager->testRender(image, run, { 3, 36 }, TestRenderFlags::None, { 3, 3 + 460 }, { 36 });
        });
        visualTestMono("wrapped-ar-left", { 490, 384 }, [&](RC<Image> image) {
            Font font{ noto, 32.f };
            // clang-format off
        auto t = U"يولد جميع الناس أحرارًا متساوين في الكرامة والحقوق. وقد وهبوا عقلاً وضميرًا وعليهم أن يعامل بعضهم بعضًا بروح الإخاء.";
            // clang-format on
            auto shaped = fontManager->shape(font, t);
            auto run    = shaped.prerender(font, 360);
            fontManager->testRender(image, run, { 23, 36 }, TestRenderFlags::None, { 23, 23 + 360 }, { 36 });
        });
        visualTestMono("wrapped-ar-right", { 490, 384 }, [&](RC<Image> image) {
            Font font{ noto, 32.f };
            // clang-format off
        auto t = U"يولد جميع الناس أحرارًا متساوين في الكرامة والحقوق. وقد وهبوا عقلاً وضميرًا وعليهم أن يعامل بعضهم بعضًا بروح الإخاء.";
            // clang-format on
            auto shaped = fontManager->shape(font, t);
            auto run    = shaped.prerender(font, 360);
            run.alignLines({ 0, 0, 360, 384 }, 1.f, 0.f);
            fontManager->testRender(image, run, { 23, 36 }, TestRenderFlags::None, { 23, 23 + 360 }, { 36 });
        });
    }
    visualTestMono("letter-spacing", { 640, 64 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        font.letterSpacing = 12.f;
        auto t             = U"Letter spacing fi fl ff áb́ć";
        auto shaped        = fontManager->shape(font, t);
        auto run           = shaped.prerender(font, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    visualTestMono("word-spacing", { 640, 64 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        font.wordSpacing = 12.f;
        auto t           = U"Word spacing fi fl ff áb́ć";
        auto shaped      = fontManager->shape(font, t);
        auto run         = shaped.prerender(font, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    visualTestMono("letter-spacing-cn", { 640, 64 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        font.letterSpacing = 12.f;
        auto t             = U"人人生而自由，在尊严和权利上一律平等。";
        auto shaped        = fontManager->shape(font, t);
        auto run           = shaped.prerender(font, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    visualTestMono("word-spacing-cn", { 640, 64 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        font.wordSpacing = 12.f;
        auto t           = U"人人生而自由，在尊严和权利上一律平等。";
        auto shaped      = fontManager->shape(font, t);
        auto run         = shaped.prerender(font, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    if (icuAvailable) {
        visualTestMono("letter-spacing-ar", { 640, 64 }, [&](RC<Image> image) {
            Font font{ noto, 22.f };
            font.letterSpacing = 12.f;
            auto t             = U"abcdef مرحبا بالعالم!";
            auto shaped        = fontManager->shape(font, t);
            auto run           = shaped.prerender(font, HUGE_VALF);
            fontManager->testRender(image, run, { 5, 36 });
        });
        visualTestMono("word-spacing-ar", { 640, 64 }, [&](RC<Image> image) {
            Font font{ noto, 22.f };
            font.wordSpacing = 12.f;
            auto t           = U"abcdef مرحبا بالعالم!";
            auto shaped      = fontManager->shape(font, t);
            auto run         = shaped.prerender(font, HUGE_VALF);
            fontManager->testRender(image, run, { 5, 36 });
        });
    }

    visualTestMono("alignment-ltr", { 256, 64 }, [&](RC<Image> image) {
        Font font{ noto, 32.f };
        auto t   = U"Hello, world!";
        auto run = fontManager->prerender(font, t);
        run.align(RectangleF(PointF(0, 0), image->size()).withPadding(2, 2), 0.f, 0.5f);
        fontManager->testRender(image, run, { 0, 0 }, TestRenderFlags::TextBounds);
    });
    if (icuAvailable) {
        visualTestMono("alignment-rtl", { 256, 64 }, [&](RC<Image> image) {
            Font font{ noto, 32.f };
            auto t   = U"مرحبا بالعالم!";
            auto run = fontManager->prerender(font, t);
            run.align(RectangleF(PointF(0, 0), image->size()).withPadding(2, 2), 1.f, 0.5f);
            fontManager->testRender(image, run, { 0, 0 }, TestRenderFlags::TextBounds);
        });
    }

    visualTestMono("wrapped-align-left", { 256, 256 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        auto t = U"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla scelerisque posuere urna "
                 U"sit amet luctus.";
        auto run = fontManager->prerender(font, t, image->width() - 4);
        run.alignLines(RectangleF(PointF(0, 0), image->size()).withPadding(2, 2), 0.f, 0.5f);
        fontManager->testRender(image, run, { 0, 0 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("wrapped-align-center", { 256, 256 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        auto t = U"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla scelerisque posuere urna "
                 U"sit amet luctus.";
        auto run = fontManager->prerender(font, t, image->width() - 4);
        run.alignLines(RectangleF(PointF(0, 0), image->size()).withPadding(2, 2), 0.5f, 0.5f);
        fontManager->testRender(image, run, { 0, 0 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("wrapped-align-right", { 256, 256 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        auto t = U"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla scelerisque posuere urna "
                 U"sit amet luctus.";
        auto run = fontManager->prerender(font, t, image->width() - 4);
        run.alignLines(RectangleF(PointF(0, 0), image->size()).withPadding(2, 2), 1.f, 0.5f);
        fontManager->testRender(image, run, { 0, 0 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("indented-left", { 256, 256 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        auto t   = U"0\n  2\n    4\n  2\n   3\n0";
        auto run = fontManager->prerender(font, t, image->width() - 4);
        run.alignLines(RectangleF(PointF(0, 0), image->size()).withPadding(2, 2), 0.f, 0.5f);
        fontManager->testRender(image, run, { 0, 0 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("indented-right", { 256, 256 }, [&](RC<Image> image) {
        Font font{ noto, 22.f };
        auto t   = U"0\n  2\n    4\n  2\n   3\n0";
        auto run = fontManager->prerender(font, t, image->width() - 4);
        run.alignLines(RectangleF(PointF(0, 0), image->size()).withPadding(2, 2), 1.f, 0.5f);
        fontManager->testRender(image, run, { 0, 0 }, TestRenderFlags::TextBounds);
    });

    fontManager.reset();
}

} // namespace Brisk
