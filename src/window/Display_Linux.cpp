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
#include <brisk/window/Display.hpp>
#include <brisk/core/Utilities.hpp>

#include <shared_mutex>
#include "X11.hpp"

namespace Brisk {

namespace X11 {
void pollDisplays();
}

class DisplayX11 final : public Display {
public:
    Point position() const {
        std::shared_lock lk(m_mutex);
        return m_rect.p1;
    }

    Rectangle workarea() const {
        std::shared_lock lk(m_mutex);
        return m_workarea;
    }

    Size resolution() const {
        return nativeResolution();
    }

    Size nativeResolution() const {
        std::shared_lock lk(m_mutex);
        return m_resolution;
    }

    Size size() const {
        return nativeResolution();
    }

    SizeF physicalSize() const {
        // No lock needed
        return m_physSize;
    }

    int dpi() const {
        return std::round(contentScale() * 96);
    }

    const std::string& name() const {
        // No lock needed
        return m_name;
    }

    const std::string& id() const {
        // No lock needed
        return m_id;
    }

    const std::string& adapterName() const {
        // No lock needed
        return m_adapterName;
    }

    const std::string& adapterId() const {
        // No lock needed
        return m_adapterId;
    }

    float contentScale() const {
        // no need to lock because contentScaleX/contentScaleY don't change
        return SizeF(X11::contentScaleX, X11::contentScaleY).longestSide();
    }

    Point desktopToMonitor(Point pt) const {
        std::shared_lock lk(m_mutex);
        return pt - m_rect.p1;
    }

    Point monitorToDesktop(Point pt) const {
        std::shared_lock lk(m_mutex);
        return pt + m_rect.p1;
    }

    DisplayFlags flags() const {
        std::shared_lock lk(m_mutex);
        return m_flags;
    }

    double refreshRate() const {
        std::shared_lock lk(m_mutex);
        return m_refreshRate;
    }

    int backingScaleFactor() const {
        return 1;
    }

    DisplayX11(RROutput output, RRCrtc crtc, Size physSize);

private:
    RROutput m_output;
    RRCrtc m_crtc;
    mutable std::shared_mutex m_mutex;
    std::string m_adapterName;
    std::string m_adapterId;
    std::string m_name;
    std::string m_id;
    Rectangle m_workarea;
    Rectangle m_rect;
    double m_refreshRate;
    DisplayFlags m_flags;
    Size m_physSize;
    Size m_resolution;
    int m_counter = 0;
    friend void X11::pollDisplays();
};

std::shared_mutex displayMutex;

namespace X11 {

// Check whether the display mode should be included in enumeration
//
static bool modeIsGood(const XRRModeInfo* mi) {
    return (mi->modeFlags & RR_Interlace) == 0;
}

// Calculates the refresh rate, in Hz, from the specified RandR mode info
//
static int calculateRefreshRate(const XRRModeInfo* mi) {
    if (mi->hTotal && mi->vTotal)
        return (int)std::round((double)mi->dotClock / ((double)mi->hTotal * (double)mi->vTotal));
    else
        return 0;
}

// Returns the mode info for a RandR mode XID
//
static const XRRModeInfo* getModeInfo(const XRRScreenResources* sr, RRMode id) {
    for (int i = 0; i < sr->nmode; i++) {
        if (sr->modes[i].id == id)
            return sr->modes + i;
    }

    return nullptr;
}

static std::map<RROutput, RC<DisplayX11>> displays;
RC<DisplayX11> primaryDisplay;

void pollDisplays() {
    initializeX11();
    SCOPE_EXIT {
        terminateX11();
    };

    XRRScreenResources* sr      = XRRGetScreenResourcesCurrent(display, root);
    RROutput primary            = XRRGetOutputPrimary(display, root);
    int screenCount             = 0;
    XineramaScreenInfo* screens = XineramaQueryScreens(display, &screenCount);

    for (int i = 0; i < sr->noutput; i++) {
        XRROutputInfo* oi = XRRGetOutputInfo(display, sr, sr->outputs[i]);
        if (oi->connection != RR_Connected || oi->crtc == None) {
            XRRFreeOutputInfo(oi);
            continue;
        }
        SCOPE_EXIT {
            XRRFreeOutputInfo(oi);
        };

        XRRCrtcInfo* ci = XRRGetCrtcInfo(display, sr, oi->crtc);
        SCOPE_EXIT {
            XRRFreeCrtcInfo(ci);
        };
        int widthMM, heightMM;
        if (ci->rotation == RR_Rotate_90 || ci->rotation == RR_Rotate_270) {
            widthMM  = oi->mm_height;
            heightMM = oi->mm_width;
        } else {
            widthMM  = oi->mm_width;
            heightMM = oi->mm_height;
        }

        if (widthMM <= 0 || heightMM <= 0) {
            // HACK: If RandR does not provide a physical size, assume the
            //       X11 default 96 DPI and calculate from the CRTC viewport
            // NOTE: These members are affected by rotation, unlike the mode
            //       info and output info members
            widthMM  = (int)(ci->width * 25.4f / 96.f);
            heightMM = (int)(ci->height * 25.4f / 96.f);
        }
        RROutput output         = sr->outputs[i];
        RRCrtc crtc             = oi->crtc;

        RC<DisplayX11>& monitor = displays[output];

        if (!monitor)
            monitor = rcnew DisplayX11(output, crtc, Size(widthMM, heightMM));
        monitor->m_resolution = Size(ci->width, ci->height);
        monitor->m_rect       = Rectangle(Point(ci->x, ci->y), monitor->m_resolution);
        monitor->m_id         = std::to_string(output);
        monitor->m_adapterId  = monitor->m_id;
        monitor->m_name       = oi->name;
        const XRRModeInfo* mi = getModeInfo(sr, ci->mode);

        Size area;

        if (ci->rotation == RR_Rotate_90 || ci->rotation == RR_Rotate_270) {
            area.width  = mi->height;
            area.height = mi->width;
        } else {
            area.width  = mi->width;
            area.height = mi->height;
        }

        if (mi->hTotal && mi->vTotal)
            monitor->m_refreshRate = (double)mi->dotClock / ((double)mi->hTotal * (double)mi->vTotal);
        else
            monitor->m_refreshRate = 0;

        if (output == primary) {
            monitor->m_flags |= DisplayFlags::Primary;
        }
        primaryDisplay      = monitor;

        monitor->m_workarea = Rectangle(Point(ci->x, ci->y), area);
        ++monitor->m_counter;
    }
}

} // namespace X11

std::vector<RC<Display>> Display::all() {
    std::shared_lock lk(displayMutex);
    std::vector<RC<Display>> result;
    for (const auto& a : X11::displays) {
        result.push_back(a.second);
    }
    return result;
}

/*static*/ RC<Display> Display::primary() {
    std::shared_lock lk(displayMutex);
    return X11::primaryDisplay;
}

void Internal::updateDisplays() {
    std::unique_lock lk(displayMutex);
    X11::pollDisplays();
}

DisplayX11::DisplayX11(RROutput output, RRCrtc crtc, Size physSize)
    : m_output(output), m_crtc(crtc), m_physSize(physSize) {}

} // namespace Brisk
