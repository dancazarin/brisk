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

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xcursor/Xcursor.h>

// The XRandR extension provides mode setting and gamma control
#include <X11/extensions/Xrandr.h>

// The Xkb extension provides improved keyboard support
#include <X11/XKBlib.h>

// The Xinerama extension provides legacy monitor indices
#include <X11/extensions/Xinerama.h>

// The XInput extension provides raw mouse motion input
#include <X11/extensions/XInput2.h>

// The Shape extension provides custom window shapes
#include <X11/extensions/shape.h>

#undef None

namespace Brisk {

constexpr inline long None = 0L;

namespace X11 {
using ::Display;
using ::Window;

extern Display* display;
extern int screen;
extern Window root;
extern XContext context;
extern float contentScaleX, contentScaleY;

bool initializeX11();
void terminateX11();

bool initialize();
void terminate();
}; // namespace X11

} // namespace Brisk
