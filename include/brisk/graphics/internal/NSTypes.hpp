#pragma once

#include <brisk/core/internal/NSTypes.hpp>

#ifdef BRISK_APPLE

#include <brisk/graphics/Geometry.hpp>

namespace Brisk {

inline PointF fromNSPoint(NSPoint pt) {
    return PointF(pt.x, pt.y);
}

inline SizeF fromNSSize(NSSize size) {
    return SizeF(size.width, size.height);
}

inline RectangleF fromNSRect(NSRect rect) {
    return RectangleF(fromNSPoint(rect.origin), fromNSSize(rect.size));
}
} // namespace Brisk

#endif
