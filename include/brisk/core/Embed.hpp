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

#include <cstdint>
#include "Bytes.hpp"

namespace Brisk {

enum class EmbeddedResourceFlags : uint32_t {
    None = 0,
    ZLib = 1,
    GZip = 2,
    LZ4  = 4,
#ifdef BRISK_HAVE_BROTLI
    Brotli = 8,
#endif
};

BRISK_FLAGS(EmbeddedResourceFlags)

template <EmbeddedResourceFlags flags>
Bytes loadResource(bytes_view data);

} // namespace Brisk
