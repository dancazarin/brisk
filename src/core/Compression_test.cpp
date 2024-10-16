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
#include <brisk/core/Compression.hpp>
#include <brisk/core/Reflection.hpp>
#include "test/LoremIpsum.hpp"
#include <brisk/core/Utilities.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

void testCompression(CompressionMethod method, string ext) {

    fs::path loremIpsumText   = tempFilePath("compression????????.txt");
    fs::path loremIpsumComp   = tempFilePath("compression????????." + ext);
    fs::path loremIpsumUncomp = tempFilePath("compression????????_r.txt");
    SCOPE_EXIT {
        std::error_code ec;
        fs::remove(loremIpsumText, ec);
        fs::remove(loremIpsumComp, ec);
        fs::remove(loremIpsumUncomp, ec);
    };

    CHECK(toStringView(compressionDecode(method, compressionEncode(method, toBytesView(loremIpsum),
                                                                   CompressionLevel::Normal))) == loremIpsum);

    REQUIRE(writeUtf8(loremIpsumText, loremIpsum).has_value());

    if (auto rd = openFileForReading(loremIpsumText)) {
        if (auto wr = openFileForWriting(loremIpsumComp)) {
            auto gz     = compressionEncoder(method, *wr, CompressionLevel::Highest);
            std::ignore = writeFromReader(gz, *rd, 8192);
        } else {
            FAIL("Cannot open compressed file for writing");
        }
    } else {
        FAIL("Cannot open file for reading");
    }

    if (auto rd = openFileForReading(loremIpsumComp)) {
        if (auto wr = openFileForWriting(loremIpsumUncomp)) {
            auto gz     = compressionDecoder(method, *rd);
            std::ignore = writeFromReader(*wr, gz, 8192);
        } else {
            FAIL("Cannot open file for writing");
        }
    } else {
        FAIL("Cannot open compressed file for reading");
    }

    CHECK(readUtf8(loremIpsumUncomp) == loremIpsum);
}

TEST_CASE("gzip") {
    testCompression(CompressionMethod::GZip, "gz");
}

TEST_CASE("zlib") {
    testCompression(CompressionMethod::ZLib, "zlib");
}

TEST_CASE("lz4") {
    testCompression(CompressionMethod::LZ4, "lz4");
}

#ifdef BRISK_HAVE_BROTLI
TEST_CASE("brotli") {
    testCompression(CompressionMethod::Brotli, "br");
}
#endif
} // namespace Brisk
