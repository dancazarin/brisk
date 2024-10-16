#pragma once

#include <brisk/core/BasicTypes.hpp>
#include <fmt/format.h>
#include <brisk/core/Reflection.hpp>
#include <type_traits>
#include <brisk/core/SIMD.hpp>
#include <array>

namespace Brisk {

/**
 * @enum ColorSpace
 * @brief Defines a set of color spaces.
 *
 * This enum class represents various color spaces, each of which operates within a
 * defined range of values for its components. These color spaces are used to represent
 * and manipulate colors in different formats, including both linear and gamma-corrected
 * forms, as well as different color representation systems like CIELAB, LMS, etc.
 */
enum class ColorSpace {
    /**
     * @brief sRGB color space in linear format.
     *
     * The sRGBLinear color space operates in the linear range, where all components
     * (R, G, B) have values between 0 and 1.
     */
    sRGBLinear,

    /**
     * @brief sRGB color space in gamma-corrected format.
     *
     * The sRGBGamma color space operates in a gamma-corrected range, where all
     * components (R, G, B) are also between 0 and 1, but corrected for gamma.
     */
    sRGBGamma,

    /**
     * @brief Display P3 color space in linear format.
     *
     * The DisplayP3Linear color space is used with displays supporting P3 gamut,
     * with all components (R, G, B) having values between 0 and 1 in a linear format.
     */
    DisplayP3Linear,

    /**
     * @brief Display P3 color space in gamma-corrected format.
     *
     * The DisplayP3Gamma color space is gamma-corrected, where the P3 display color
     * components (R, G, B) have values between 0 and 1.
     */
    DisplayP3Gamma,

    /**
     * @brief CIE XYZ color space.
     *
     * The CIEXYZ color space represents color based on the CIE 1931 XYZ color model.
     * The X, Y, and Z components have ranges between 0 and 100.
     */
    CIEXYZ,

    /**
     * @brief CIE LAB color space.
     *
     * The CIELAB color space is used to approximate human vision, with the L component
     * ranging from 0 to 100, and the a and b components ranging from -200 to +200.
     */
    CIELAB,

    /**
     * @brief CIE LCH color space.
     *
     * The CIELCH color space is based on the cylindrical representation of the CIELAB color
     * model. The L component ranges from 0 to 100, the C component from 0 to 100, and
     * the H component from 0 to 360 degrees.
     */
    CIELCH,

    /**
     * @brief OKLAB color space.
     *
     * The OKLAB color space is another perceptually uniform color model, with the L
     * component ranging from 0 to 100, and the a and b components ranging from -200 to +200.
     */
    OKLAB,

    /**
     * @brief OKLCH color space.
     *
     * The OKLCH color space is a cylindrical version of the OKLAB model. The L component
     * ranges from 0 to 100, the C component from 0 to 100, and the H component from
     * 0 to 360 degrees.
     */
    OKLCH,

    /**
     * @brief LMS color space.
     *
     * The LMS color space is based on the response of the human eye's long, medium,
     * and short-wavelength cones. All components (L, M, S) have values between 0 and 1.
     */
    LMS,
};

template <>
inline constexpr std::initializer_list<NameValuePair<ColorSpace>> defaultNames<ColorSpace>{
    { "sRGBLinear", ColorSpace::sRGBLinear },
    { "sRGBGamma", ColorSpace::sRGBGamma },
    { "DisplayP3Linear", ColorSpace::DisplayP3Linear },
    { "DisplayP3Gamma", ColorSpace::DisplayP3Gamma },
    { "CIEXYZ", ColorSpace::CIEXYZ },
    { "CIELAB", ColorSpace::CIELAB },
    { "CIELCH", ColorSpace::CIELCH },
    { "OKLAB", ColorSpace::OKLAB },
    { "OKLCH", ColorSpace::OKLCH },
    { "LMS", ColorSpace::LMS },
};

namespace Internal {

// Returns the next colorspace in chain towards XYZ
constexpr ColorSpace nextColorSpace(ColorSpace space) {
    switch (space) {
    case ColorSpace::CIELCH:
        return ColorSpace::CIELAB;
    case ColorSpace::OKLCH:
        return ColorSpace::OKLAB;
    case ColorSpace::OKLAB:
        return ColorSpace::LMS;
    case ColorSpace::sRGBGamma:
        return ColorSpace::sRGBLinear;
    case ColorSpace::DisplayP3Gamma:
        return ColorSpace::DisplayP3Linear;
    default:
        return ColorSpace::CIEXYZ;
    }
}

template <ColorSpace Space, typename T>
constexpr Range<SIMD<T, 3>> colorRange() {
    if constexpr (Space == ColorSpace::CIEXYZ)
        return {
            SIMD<T, 3>{ T(0), T(0), T(0) },
            SIMD<T, 3>{ T(100), T(100), T(100) },
        };
    else if constexpr (Space == ColorSpace::CIELAB || Space == ColorSpace::OKLAB)
        return {
            SIMD<T, 3>{ T(0), T(-200), T(-200) },
            SIMD<T, 3>{ T(100), T(200), T(200) },
        };
    else if constexpr (Space == ColorSpace::CIELCH || Space == ColorSpace::OKLCH)
        return {
            SIMD<T, 3>{ T(0), T(0), T(0) },
            SIMD<T, 3>{ T(100), T(100), T(360) },
        };
    else // sRGB, DisplayP3, LMS
        return {
            SIMD<T, 3>{ T(0), T(0), T(0) }, //
            SIMD<T, 3>{ T(1), T(1), T(1) },
        };
}

} // namespace Internal

template <typename T, ColorSpace Space>
struct Trichromatic;

/**
 * @enum ColorConversionMode
 * @brief Defines the modes for adjusting colors after conversion.
 *
 * This enum class provides different strategies for handling colors that may fall outside
 * the valid range of a given color space during conversion.
 */
enum class ColorConversionMode {
    /**
     * @brief No adjustment to the color.
     *
     * The color is returned as-is, even if its values are out of the acceptable range for
     * the current color space.
     */
    None,

    /**
     * @brief Clamps the color to the valid range.
     *
     * The color is adjusted by clamping each component to the nearest boundary of the valid
     * range for the current color space.
     */
    Clamp,

    /**
     * @brief Adjusts the color to the nearest valid value by reducing chroma.
     *
     * The color is adjusted by reducing its chroma (saturation) to bring it within the
     * valid range of the color space.
     */
    Nearest,
};

template <ColorSpace DestSpace, typename T, ColorSpace Space>
Trichromatic<T, DestSpace> convertColorSpace(Trichromatic<T, Space> color,
                                             ColorConversionMode mode = ColorConversionMode::None);

template <typename T, ColorSpace Space>
struct Trichromatic {
    static_assert(std::is_floating_point_v<T>);

    constexpr Trichromatic() : value{ T(0), T(0), T(0) } {}

    constexpr Trichromatic(const Trichromatic&)            = default;
    constexpr Trichromatic& operator=(const Trichromatic&) = default;

    constexpr Trichromatic(T c1, T c2, T c3) : value{ c1, c2, c3 } {}

    template <ColorSpace SrcSpace>
    Trichromatic(Trichromatic<T, SrcSpace> color) {
        *this = convertColorSpace<Space>(color);
    }

    decltype(auto) operator[](size_t index) const {
        return value[index];
    }

    decltype(auto) operator[](size_t index) {
        return value[index];
    }

    constexpr Trichromatic clamped() const {
        auto range = Internal::colorRange<Space, T>();
        Trichromatic result;
        result.value = clamp(value, range.min, range.max);
        return result;
    }

    constexpr bool isValid() const {
        return clamped() == *this;
    }

    constexpr bool operator==(Trichromatic other) const {
        return value == other.value;
    }

    constexpr bool operator!=(Trichromatic other) const {
        return !operator==(other);
    }

    union {
        SIMD<T, 3> value;
        std::array<T, 3> array;
    };
};

enum class Illuminant {
    D50 = 0,
    D55,
    D65,
    D75,
    E,
};

constexpr std::underlying_type_t<Illuminant> operator+(Illuminant value) {
    return static_cast<std::underlying_type_t<Illuminant>>(value);
}

namespace Internal {

template <typename T>
[[maybe_unused]] constexpr SIMD<T, 3> illuminants[] = {
    { 96.422, 100.000, 82.521 },  // D50 illuminant, 2° observer
    { 95.682, 100.000, 92.149 },  // D55 illuminant, 2° observer
    { 95.047, 100.000, 108.883 }, // D65 illuminant, 2° observer
    { 94.972, 100.000, 122.638 }, // D75 illuminant, 2° observer
    { 100, 100, 100 },            // E illuminant, 2° observer
};

template <typename T, ColorSpace Space>
inline void convertColorSpace(Trichromatic<T, Space>& dest, Trichromatic<T, Space> color) {
    // no conversion needed
    dest = color;
}

// xyz_to_rgb
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::sRGBLinear>& rgb,
                       Trichromatic<T, ColorSpace::CIEXYZ> xyz) {
    xyz.value *= T(0.01);
    rgb.value = xyz[0] * SIMD<T, 3>{ T(3.2406), T(-0.9689), T(0.0557) } +
                xyz[1] * SIMD<T, 3>{ T(-1.5372), T(1.8758), T(-0.2040) } +
                xyz[2] * SIMD<T, 3>{ T(-0.4986), T(0.0415), T(1.0570) };
}

// rgb_to_xyz
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::CIEXYZ>& xyz,
                       Trichromatic<T, ColorSpace::sRGBLinear> rgb) {
    xyz.value = (rgb[0] * SIMD<T, 3>{ T(0.4124), T(0.2126), T(0.0193) } +
                 rgb[1] * SIMD<T, 3>{ T(0.3576), T(0.7152), T(0.1192) } +
                 rgb[2] * SIMD<T, 3>{ T(0.1805), T(0.0722), T(0.9505) });
    xyz.value *= T(100);
}

// xyz_to_p3
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::DisplayP3Linear>& p3,
                       Trichromatic<T, ColorSpace::CIEXYZ> xyz) {
    xyz.value *= T(0.01);
    p3.value = xyz[0] * SIMD<T, 3>{ 2.493498, -0.82949, 0.035846 } +
               xyz[1] * SIMD<T, 3>{ -0.931385, 1.762664, -0.076172 } +
               xyz[2] * SIMD<T, 3>{ -0.40271, 0.023625, 0.956885 };
}

// p3_to_xyz
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::CIEXYZ>& xyz,
                       Trichromatic<T, ColorSpace::DisplayP3Linear> p3) {
    xyz.value =
        (p3[0] * SIMD<T, 3>{ 0.486571, 0.228975, -0. } + p3[1] * SIMD<T, 3>{ 0.265668, 0.691739, 0.045113 } +
         p3[2] * SIMD<T, 3>{ 0.198217, 0.079287, 1.043944 });
    xyz.value *= T(100);
}

// xyz_to_lab
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::CIELAB>& lab, Trichromatic<T, ColorSpace::CIEXYZ> xyz) {
    xyz.value /= SIMD<T, 3>(illuminants<T>[+Illuminant::D65]);
    const SIMD<T, 3> w =
        select(gt(xyz.value, SIMD<T, 3>(T(0.008856))), pow(xyz.value, SIMD<T, 3>(T(0.33333))),
               (T(7.787) * xyz.value) + T(16) / T(116));

    lab.value = SIMD<T, 3>{ (T(116) * w[1]) - T(16), T(500) * (w[0] - w[1]), T(200) * (w[1] - w[2]) };
}

// lab_to_xyz
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::CIEXYZ>& xyz, Trichromatic<T, ColorSpace::CIELAB> lab) {
    const float y         = (lab[0] + T(16)) / T(116);
    const float x         = lab[1] / T(500) + y;
    const float z         = y - lab[2] / T(200);

    const SIMD<T, 3> w    = { x, y, z };

    const SIMD<T, 3> cube = w * w * w;
    xyz.value             = SIMD<T, 3>{ T(0.95047), T(1.00000), T(1.08883) } *
                (select(gt(cube, SIMD<T, 3>(T(216)) / T(24389)), cube,
                        (w - T(16) / T(116)) / (T(24389) / T(27) / T(116))));
    xyz.value *= T(100);
}

template <typename T>
static T fixHue(T value) {
    if (value < 0)
        return 360 + value;
    return value;
}

// lab_to_lch
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::CIELCH>& lch,
                       const Trichromatic<T, ColorSpace::CIELAB> lab) {
    lch.value = SIMD<T, 3>{
        lab[0],
        std::sqrt(lab[1] * lab[1] + lab[2] * lab[2]),
        fixHue(std::atan2(lab[2], lab[1]) * rad2deg<T>),
    };
}

// lch_to_lab
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::CIELAB>& lab,
                       const Trichromatic<T, ColorSpace::CIELCH> lch) {
    lab.value = SIMD<T, 3>{
        lch[0],
        std::cos(lch[2] * deg2rad<T>) * lch[1],
        std::sin(lch[2] * deg2rad<T>) * lch[1],
    };
}

// xyz_to_lms
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::LMS>& lms, Trichromatic<T, ColorSpace::CIEXYZ> xyz) {
    xyz.value *= T(0.01);
    lms.value = xyz[0] * SIMD<T, 3>{ T(+0.8189330101), T(+0.0329845436), T(+0.0482003018) } +
                xyz[1] * SIMD<T, 3>{ T(+0.3618667424), T(+0.9293118715), T(+0.2643662691) } +
                xyz[2] * SIMD<T, 3>{ T(-0.1288597137), T(+0.0361456387), T(+0.6338517070) };
}

// lms_to_xyz
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::CIEXYZ>& xyz, Trichromatic<T, ColorSpace::LMS> lms) {
    xyz.value = (lms[0] * SIMD<T, 3>{ T(1.2270138511), T(-0.0405801784), T(-0.0763812845) } +
                 lms[1] * SIMD<T, 3>{ T(-0.5577999806), T(1.1122568696), T(-0.4214819784) } +
                 lms[2] * SIMD<T, 3>{ T(0.2812561490), T(-0.0716766787), T(1.5861632204) });
    xyz.value *= T(100);
}

// lms_to_oklab
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::OKLAB>& oklab, Trichromatic<T, ColorSpace::LMS> lms) {
    lms.value   = copysign(pow(abs(lms.value), SIMD<T, 3>(T(0.33333333))), lms.value);
    oklab.value = (lms[0] * SIMD<T, 3>{ T(0.2104542553), T(1.9779984951), T(0.0259040371) } +
                   lms[1] * SIMD<T, 3>{ T(0.7936177850), T(-2.4285922050), T(0.7827717662) } +
                   lms[2] * SIMD<T, 3>{ T(-0.0040720468), T(0.4505937099), T(-0.8086757660) });
    oklab.value *= T(100);
}

// oklab_to_lms
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::LMS>& lms, Trichromatic<T, ColorSpace::OKLAB> oklab) {
    oklab.value *= T(0.01);
    lms.value = (oklab[0] + //
                 oklab[1] * SIMD<T, 3>{ T(0.3963377774), T(-0.1055613458), T(-0.0894841775) } +
                 oklab[2] * SIMD<T, 3>{ T(0.2158037573), T(-0.0638541728), T(-1.2914855480) });
    lms.value = pow(lms.value, SIMD<T, 3>(3));
}

// oklab_to_oklch
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::OKLCH>& lch,
                       const Trichromatic<T, ColorSpace::OKLAB> lab) {
    lch.value = SIMD<T, 3>{ lab[0], std::sqrt(lab[1] * lab[1] + lab[2] * lab[2]),
                            fixHue(std::atan2(lab[2], lab[1]) * rad2deg<T>) };
}

// oklch_to_oklab
template <typename T>
void convertColorSpace(Trichromatic<T, ColorSpace::OKLAB>& lab,
                       const Trichromatic<T, ColorSpace::OKLCH> lch) {
    lab.value =
        SIMD<T, 3>{ lch[0], std::cos(lch[2] * deg2rad<T>) * lch[1], std::sin(lch[2] * deg2rad<T>) * lch[1] };
}

/////////////////////////////

template <typename T, size_t N>
constexpr BRISK_INLINE SIMD<T, N> srgbGammaToLinear(SIMD<T, N> x) {
    static_assert(std::is_floating_point_v<T>);
    SIMD<T, N> v = abs(x);
    if (std::is_constant_evaluated()) {
        v = v * (v * (v * T(0.305306011) + T(0.682171111)) + T(0.012522878));
    } else {
        SIMDMask<N> m = le(v, SIMD<T, N>(0.04045));
        v             = select(m, v * SIMD<T, N>(0.07739938080495356),
                               pow((v + T(0.055)) * std::nexttoward(T(0.947867298578199), T(1)), SIMD<T, N>(2.4)));
    }
    return copysign(v, x);
}

template <typename T, size_t N>
constexpr BRISK_INLINE SIMD<T, N> srgbLinearToGamma(SIMD<T, N> x) {
    static_assert(std::is_floating_point_v<T>);
    SIMD<T, N> v = abs(x);
    if (std::is_constant_evaluated()) {
        SIMD<T, N> S1 = sqrt(v);
        SIMD<T, N> S2 = sqrt(S1);
        SIMD<T, N> S3 = sqrt(S2);
        v             = T(0.585122381) * S1 + T(0.783140355) * S2 - T(0.368262736) * S3;
    } else {
        SIMDMask<N> m = le(v, SIMD<T, N>(T(0.0031308)));
        v             = select(m, v * T(12.92),
                               T(1.055) * pow(v, SIMD<T, N>(std::nexttoward(T(0.416666666666667), T(0)))) - T(0.055));
    }
    return copysign(v, x);
}

template <std::floating_point T>
BRISK_INLINE T srgbGammaToLinear(T v) {
    return srgbGammaToLinear(SIMD{ v }).front();
}

template <std::floating_point T>
BRISK_INLINE T srgbLinearToGamma(T v) {
    return srgbLinearToGamma(SIMD{ v }).front();
}

// rgb_to_rgbgam
template <typename T>
BRISK_INLINE void convertColorSpace(Trichromatic<T, ColorSpace::sRGBGamma>& rgbgam,
                                    Trichromatic<T, ColorSpace::sRGBLinear> rgblin) {
    rgbgam.value = srgbLinearToGamma(rgblin.value);
}

// rgbgam_to_rgb
template <typename T>
BRISK_INLINE void convertColorSpace(Trichromatic<T, ColorSpace::sRGBLinear>& rgblin,
                                    Trichromatic<T, ColorSpace::sRGBGamma> rgbgam) {
    rgblin.value = srgbGammaToLinear(rgbgam.value);
}

// p3_to_p3gam
template <typename T>
BRISK_INLINE void convertColorSpace(Trichromatic<T, ColorSpace::DisplayP3Gamma>& p3gam,
                                    Trichromatic<T, ColorSpace::DisplayP3Linear> p3lin) {
    p3gam.value = srgbLinearToGamma(p3lin.value);
}

// p3gam_to_p3
template <typename T>
BRISK_INLINE void convertColorSpace(Trichromatic<T, ColorSpace::DisplayP3Linear>& p3lin,
                                    Trichromatic<T, ColorSpace::DisplayP3Gamma> p3gam) {
    p3lin.value = srgbGammaToLinear(p3gam.value);
}

// anything to anything
template <typename T, ColorSpace Space, ColorSpace DestSpace>
BRISK_INLINE void convertColorSpace(Trichromatic<T, DestSpace>& dest, Trichromatic<T, Space> color) {
    if constexpr (Space == ColorSpace::CIEXYZ) {
        // From XYZ
        Trichromatic<T, nextColorSpace(DestSpace)> tmp;
        convertColorSpace(tmp, color);
        convertColorSpace(dest, tmp);
    } else if constexpr (DestSpace == ColorSpace::CIEXYZ) {
        // To XYZ
        Trichromatic<T, nextColorSpace(Space)> tmp;
        convertColorSpace(tmp, color);
        convertColorSpace(dest, tmp);
    } else {
        Trichromatic<T, ColorSpace::CIEXYZ> tmp;
        convertColorSpace(tmp, color);
        convertColorSpace(dest, tmp);
    }
}

} // namespace Internal

template <ColorSpace DestSpace, typename T, ColorSpace Space>
BRISK_INLINE Trichromatic<T, DestSpace> convertColorSpace(Trichromatic<T, Space> color,
                                                          ColorConversionMode mode) {
    Trichromatic<T, DestSpace> result;
    Internal::convertColorSpace(result, color);
    switch (mode) {
    case ColorConversionMode::Clamp:
        return result.clamped();
    case ColorConversionMode::Nearest:
        if (result.isValid())
            return result;
        if constexpr (Space == ColorSpace::CIELAB || Space == ColorSpace::CIELCH ||
                      Space == ColorSpace::OKLAB || Space == ColorSpace::OKLCH) {
            T lowest  = T(0);
            T highest = T(1);
            for (int i = 0; i < 10; ++i) {
                T middle                   = (lowest + highest) * T(0.5);
                Trichromatic<T, Space> tmp = color;
                if constexpr (Space == ColorSpace::CIELAB || Space == ColorSpace::OKLAB) {
                    tmp[1] *= middle; // a
                    tmp[2] *= middle; // b
                } else {
                    tmp[1] *= middle; // C
                }
                Internal::convertColorSpace(result, tmp);
                if (result.isValid()) {
                    lowest = middle;
                } else {
                    highest = middle;
                }
            }
            return result.clamped();
        } else {
            // no chroma in source color, convert to L*ab
            Trichromatic<T, ColorSpace::CIELAB> tmp;
            Internal::convertColorSpace(tmp, color);
            return convertColorSpace<DestSpace>(tmp, ColorConversionMode::Nearest);
        }

    default:
        return result;
    }
}

template <typename T = float>
Trichromatic<T, ColorSpace::CIEXYZ> illuminant(Illuminant illum) {
    Trichromatic<T, ColorSpace::CIEXYZ> result;
    result.value = Internal::illuminants<T>[+illum];
    return result;
}

using ColorSRGBLinear      = Trichromatic<float, ColorSpace::sRGBLinear>;
using ColorSRGBGamma       = Trichromatic<float, ColorSpace::sRGBGamma>;
using ColorDisplayP3Linear = Trichromatic<float, ColorSpace::DisplayP3Linear>;
using ColorDisplayP3Gamma  = Trichromatic<float, ColorSpace::DisplayP3Gamma>;
using ColorCIEXYZ          = Trichromatic<float, ColorSpace::CIEXYZ>;
using ColorCIELAB          = Trichromatic<float, ColorSpace::CIELAB>;
using ColorCIELCH          = Trichromatic<float, ColorSpace::CIELCH>;
using ColorOKLAB           = Trichromatic<float, ColorSpace::OKLAB>;
using ColorOKLCH           = Trichromatic<float, ColorSpace::OKLCH>;
using ColorLMS             = Trichromatic<float, ColorSpace::LMS>;

} // namespace Brisk

template <typename T, Brisk::ColorSpace Space>
struct fmt::formatter<Brisk::Trichromatic<T, Space>> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::Trichromatic<T, Space>& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{}{{ {}, {}, {} }}", Space, value[0], value[1], value[2]), ctx);
    }
};
