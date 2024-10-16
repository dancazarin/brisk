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
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include <brisk/graphics/Matrix.hpp>
#include <random>

namespace Catch {
namespace Matchers {

class PointWithinAbsMatcher final : public MatcherBase<Brisk::PointF> {
public:
    PointWithinAbsMatcher(const Brisk::PointF& target, double margin) : m_target(target), m_margin(margin) {}

    bool match(const Brisk::PointF& matchee) const override {
        return (matchee.x + m_margin >= m_target.x) && (m_target.x + m_margin >= matchee.x) &&
               (matchee.y + m_margin >= m_target.y) && (m_target.y + m_margin >= matchee.y);
    }

    std::string describe() const override {
        return "is within " + ::Catch::Detail::stringify(m_margin) + " of " +
               ::Catch::Detail::stringify(m_target);
    }

private:
    Brisk::PointF m_target;
    double m_margin;
};
} // namespace Matchers
} // namespace Catch

namespace Brisk {

TEST_CASE("matrix") {

    constexpr float sqrt05 = std::numbers::sqrt2_v<float> / 2;

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::translation(100, -1000) ==
          Matrix2D{ 1, 2, 3, 4, 105, -994 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.translate(100, -1000) == Matrix2D{ 1, 2, 3, 4, 105, -994 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::scaling(10, 100) == Matrix2D{ 10, 200, 30, 400, 50, 600 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.scale(10, 100) == Matrix2D{ 10, 200, 30, 400, 50, 600 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::skewness(1, -1) == Matrix2D{ 3, 1, 7, 1, 11, 1 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.skew(1, -1) == Matrix2D{ 3, 1, 7, 1, 11, 1 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::skewness(0, -1) == Matrix2D{ 1, 1, 3, 1, 5, 1 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.skew(0, -1) == Matrix2D{ 1, 1, 3, 1, 5, 1 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::rotation(180) == Matrix2D{ -1, -2, -3, -4, -5, -6 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.rotate(180) == Matrix2D{ -1, -2, -3, -4, -5, -6 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::rotation90(2) == Matrix2D{ -1, -2, -3, -4, -5, -6 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.rotate90(2) == Matrix2D{ -1, -2, -3, -4, -5, -6 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::rotation(180 / 2) == Matrix2D{ -2, 1, -4, 3, -6, 5 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.rotate(180 / 2) == Matrix2D{ -2, 1, -4, 3, -6, 5 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::rotation90(1) == Matrix2D{ -2, 1, -4, 3, -6, 5 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.rotate90(1) == Matrix2D{ -2, 1, -4, 3, -6, 5 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::rotation90(3) == Matrix2D{ 2, -1, 4, -3, 6, -5 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.rotate90(3) == Matrix2D{ 2, -1, 4, -3, 6, -5 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D::rotation(180 / 4) ==
          Matrix2D{ -sqrt05, 3 * sqrt05, -sqrt05, 7 * sqrt05, -sqrt05, 11 * sqrt05 });
    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 }.rotate(180 / 4) ==
          Matrix2D{ -sqrt05, 3 * sqrt05, -sqrt05, 7 * sqrt05, -sqrt05, 11 * sqrt05 });

    CHECK(Matrix2D{ 1, 2, 3, 4, 5, 6 } * Matrix2D{ 10, 20, 30, 40, 50, 60 } ==
          Matrix2D{ 70, 100, 150, 220, 280, 400 });
    CHECK(Matrix2D{ 10, 20, 30, 40, 50, 60 } * Matrix2D{ 1, 2, 3, 4, 5, 6 } ==
          Matrix2D{ 70, 100, 150, 220, 235, 346 });

    CHECK(PointF{ 12.f, 34.f } * Matrix2D() == PointF{ 12.f, 34.f });
    CHECK(PointF{ 12.f, 34.f } * Matrix2D::translation(100.f, -1.f) == PointF{ 112.f, 33.f });
    CHECK(PointF{ 12.f, 34.f } * Matrix2D::scaling(2.f, 0.5f) == PointF{ 24.f, 17.f });
    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation(180 * 0 / 2),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 12, 34 }, 0.001));
    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation(180 * 1 / 2),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ -34, 12 }, 0.001));
    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation(180 * 2 / 2),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ -12, -34 }, 0.001));
    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation(180 * 3 / 2),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 34, -12 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation90(0),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 12, 34 }, 0.001));
    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation90(1),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ -34, 12 }, 0.001));
    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation90(2),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ -12, -34 }, 0.001));
    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::rotation90(3),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 34, -12 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * (Matrix2D::rotation90(1) * Matrix2D::translation(10.f, -1.f)),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ -24, 11 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * (Matrix2D::translation(10.f, -1.f) * Matrix2D::rotation90(1)),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ -33, 22 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * (Matrix2D::translation(10.f, -1.f) * Matrix2D::scaling(2, 2)),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 44, 66 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * (Matrix2D::scaling(2, 2) * Matrix2D::translation(10.f, -1.f)),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 34, 67 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * (Matrix2D::rotation(180 / 4) * Matrix2D::rotation(180 / 4)),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ -34, 12 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::skewness(1, 0),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 46, 34 }, 0.001));

    CHECK_THAT(PointF(12.f, 34.f) * Matrix2D::skewness(0, 1),
               Catch::Matchers::PointWithinAbsMatcher(PointF{ 12, 46 }, 0.001));

    std::array<PointF, 31> pts;
    std::mt19937 rnd(123456);
    std::uniform_real_distribution<float> u{ -10.f, +10.f };
    for (size_t i = 0; i < pts.size(); ++i) {
        pts[i].x = u(rnd);
        pts[i].y = u(rnd);
    }
    auto original_pts = pts;
    Matrix2D m        = Matrix2D{}.scale(1.1f, 0.9f).rotate(1.9f).skew(-0.5f, 0.1f).translate(-10.f, +4.f);
    m.transform(pts);
    for (size_t i = 0; i < pts.size(); ++i) {
        INFO(i);
        CHECK_THAT(m.transform(original_pts[i]), Catch::Matchers::PointWithinAbsMatcher(pts[i], 0.001));
    }
}
} // namespace Brisk
