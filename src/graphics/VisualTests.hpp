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
#include <catch2/catch_all.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/graphics/Image.hpp>
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/graphics/Color.hpp>

namespace Brisk {

[[maybe_unused]] static float imagePSNR(RC<Image> img, RC<Image> ref) {
    Image::AccessR rimg = img->mapRead();
    Image::AccessR rref = ref->mapRead();
    REQUIRE(rimg.components() == rref.components());
    REQUIRE(rimg.width() == rref.width());
    REQUIRE(rimg.height() == rref.height());
    double sumsqr = 0;
    for (int y = 0; y < rimg.height(); ++y) {
        for (int x = 0; x < rimg.memoryWidth(); ++x) {
            sumsqr += std::pow((rimg.line(y)[x] - rref.line(y)[x]), 2);
        }
    }
    double mse  = sumsqr / rimg.memorySize();
    double maxI = 255.f;
    double psnr = 10 * std::log10(maxI * maxI / mse);
    return psnr;
}

template <PixelFormat Format = PixelFormat::RGBA, typename Fn>
static void visualTest(const std::string& referenceImageName, Size size, Fn&& fn, float minimumPSNR = 40.f) {
    INFO(referenceImageName);
    Pixel<uint8_t, Format> clearValue = colorToPixel<PixelType::U8Gamma, Format>(Color(255, 255, 255, 255));
    RC<ImageTyped<PixelType::U8Gamma, Format>> testImage =
        rcnew ImageTyped<PixelType::U8Gamma, Format>(size, clearValue);
    bool testOk = false;
    SCOPE_EXIT {
        if (!testOk) {
            fs::path tmpPath = tempFilePath(referenceImageName + ".png");
            WARN("PNG saved at " << tmpPath.string());
            REQUIRE(writeBytes(tmpPath, pngEncode(testImage)));
        }
    };

    fn(testImage);

    fs::path fileName =
        fs::path(PROJECT_SOURCE_DIR) / "src" / "graphics" / "testdata" / (referenceImageName + ".png");
    auto refImgBytes = readBytes(fileName);
    CHECK(refImgBytes.has_value());
    if (refImgBytes.has_value()) {
        expected<RC<Image>, ImageIOError> decodedRefImg = pngDecode(*refImgBytes);
        REQUIRE(decodedRefImg.has_value());
        REQUIRE((*decodedRefImg)->size() == size);
        REQUIRE((*decodedRefImg)->format() == Format);
        RC<ImageTyped<PixelType::U8Gamma, Format>> decodedRefImgT =
            (*decodedRefImg)->as<PixelType::U8Gamma, Format>();
        float testPSNR = imagePSNR(testImage, decodedRefImgT);
        CHECK(testPSNR > minimumPSNR);
        testOk = testPSNR > minimumPSNR;
    }
}

template <typename Fn>
static void visualTestMono(const std::string& referenceImageName, Size size, Fn&& fn,
                           float minimumPSNR = 40.f) {
    visualTest<PixelFormat::Greyscale>(referenceImageName, size, std::forward<Fn>(fn), minimumPSNR);
}

} // namespace Brisk
