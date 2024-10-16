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
#include <brisk/core/Embed.hpp>
#include <brisk/core/Compression.hpp>

namespace Brisk {

template <EmbeddedResourceFlags flags>
Bytes loadResource(bytes_view data) {
    Bytes result(data.data(), data.data() + data.size());
    if constexpr (flags && EmbeddedResourceFlags::ZLib) {
        result = zlibDecode(result);
    }
    if constexpr (flags && EmbeddedResourceFlags::GZip) {
        result = gzipDecode(result);
    }
    if constexpr (flags && EmbeddedResourceFlags::LZ4) {
        result = lz4Decode(result);
    }
#ifdef BRISK_HAVE_BROTLI
    if constexpr (flags && EmbeddedResourceFlags::Brotli) {
        result = brotliDecode(result);
    }
#endif
    return result;
}

template Bytes loadResource<EmbeddedResourceFlags::None>(bytes_view);
template Bytes loadResource<EmbeddedResourceFlags::ZLib>(bytes_view);
template Bytes loadResource<EmbeddedResourceFlags::GZip>(bytes_view);
template Bytes loadResource<EmbeddedResourceFlags::LZ4>(bytes_view);
#ifdef BRISK_HAVE_BROTLI
template Bytes loadResource<EmbeddedResourceFlags::Brotli>(bytes_view);
#endif

} // namespace Brisk
