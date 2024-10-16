#pragma once

#include "Image.hpp"
#include "Color.hpp"

namespace Brisk {

namespace Internal {
struct SVGImpl;
}

/**
 * @struct SVGImage
 * @brief A class to represent and render SVG images.
 *
 * This class provides functionality to load an SVG image from a string and
 * render it as a raster image with a specified size and background color.
 */
struct SVGImage {
public:
    /**
     * @brief Constructs an SVGImage from a given SVG string.
     *
     * @param svg A string view representing the SVG data.
     */
    SVGImage(std::string_view svg);
    SVGImage(bytes_view svg);
    SVGImage(const SVGImage&) noexcept            = default;
    SVGImage(SVGImage&&) noexcept                 = default;
    SVGImage& operator=(const SVGImage&) noexcept = default;
    SVGImage& operator=(SVGImage&&) noexcept      = default;

    /**
     * @brief Destructor for the SVGImage class.
     */
    ~SVGImage();

    /**
     * @brief Renders the SVG image to an RGBA format.
     *
     * @param size The desired size of the output image.
     * @param background The background color to use (default is transparent).
     *
     * @return A smart pointer to an ImageRGBA object representing the rendered image.
     */
    RC<ImageRGBA> render(Size size, ColorF background = ColorF(0.f, 0.f)) const;

private:
    RC<Internal::SVGImpl> m_impl; ///< Pointer to the internal SVG implementation.
};

} // namespace Brisk
