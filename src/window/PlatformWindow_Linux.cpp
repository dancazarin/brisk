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
#include "KeyCodes.hpp"
#include "PlatformWindow.hpp"

#include <brisk/core/Log.hpp>

#include "X11.hpp"
#include "brisk/core/Utilities.hpp"
#include "brisk/core/App.hpp"
#include "brisk/core/Text.hpp"
#include "brisk/window/Display.hpp"
#include "brisk/graphics/OSWindowHandle.hpp"
#include "brisk/window/Window.hpp"
#include <fcntl.h>
#include <poll.h>

namespace Brisk {

extern std::array<KeyCode, numScanCodes> scanCodeToKeyCodeTable;
extern std::array<int16_t, numKeyCodes> keyCodeToScanCodeTable;

constexpr int XDND_VERSION         = 5;

// Action for EWMH client messages
constexpr int NET_WM_STATE_REMOVE_ = 0;
constexpr int NET_WM_STATE_ADD_    = 1;
constexpr int NET_WM_STATE_TOGGLE_ = 2;

struct PlatformWindowData {
    ::Window handle;
    ::Window parent;
    ::XIC ic;
    ::Colormap colormap;
    // Cached position and size used to filter out duplicate events
    int width, height;
    int xpos, ypos;
    // The last received cursor position, regardless of source
    Point lastCursorPos;
    Point warpCursorPos;
    // The time of the last KeyPress event per keycode, for discarding
    // duplicate key events generated for some keys by ibus
    ::Time keyPressTimes[256];

    bool maximized = false;
    bool iconified = false;
};

namespace X11 {

static bool initialized = false;

int emptyEventPipe[2];

Display* display;
int screen;
Window root;
XContext context;
float contentScaleX, contentScaleY;
Window helperWindowHandle;
XIM im;

Atom TARGETS;
Atom MULTIPLE;
Atom INCR;
Atom CLIPBOARD;
Atom PRIMARY;
Atom CLIPBOARD_MANAGER;
Atom SAVE_TARGETS;
Atom NULL_;
Atom UTF8_STRING;
Atom COMPOUND_STRING;
Atom ATOM_PAIR;
Atom NET_SUPPORTED;
Atom MOTIF_WM_HINTS;
Atom NET_WM_STATE;
Atom NET_WM_STATE_MAXIMIZED_VERT;
Atom NET_WM_STATE_MAXIMIZED_HORZ;
Atom NET_WM_STATE_ABOVE;
Atom WM_DELETE_WINDOW;
Atom NET_WM_PING;
Atom NET_WM_PID;
Atom NET_WM_WINDOW_TYPE;
Atom NET_WM_WINDOW_TYPE_NORMAL;
Atom XdndAware;
Atom XdndEnter;
Atom XdndPosition;
Atom XdndStatus;
Atom XdndActionCopy;
Atom XdndDrop;
Atom XdndFinished;
Atom XdndSelection;
Atom XdndTypeList;
Atom text_uri_list;
Atom NET_WM_NAME;
Atom NET_WM_ICON_NAME;
Atom NET_ACTIVE_WINDOW;
Atom WM_PROTOCOLS;
Atom WM_STATE;

struct {
    int eventBase;
    int errorBase;
    int major;
    int minor;
    bool monitorBroken = false;
} randr;

struct {
    bool available = false;
    bool detectable;
    int majorOpcode;
    int eventBase;
    int errorBase;
    int major;
    int minor;
    unsigned int group;
} xkb;

struct {
    int version;
    Window source;
    Atom format;
} xdnd;

// Primary selection string (while the primary selection is owned)
char* primarySelectionString;
// Clipboard string (while the selection is owned)
char* clipboardString;

Window createHelperWindow();
void pushSelectionToManagerX11();
static bool waitForX11Event(double* timeout);

// Check whether the IM has a usable style
//
bool hasUsableInputMethodStyle() {
    bool found        = false;
    XIMStyles* styles = NULL;

    if (XGetIMValues(im, XNQueryInputStyle, &styles, NULL) != NULL)
        return false;

    for (unsigned int i = 0; i < styles->count_styles; i++) {
        if (styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing)) {
            found = true;
            break;
        }
    }

    XFree(styles);
    return found;
}

static void createInputContext(PlatformWindow* window);

void inputMethodInstantiateCallback(Display* display, XPointer clientData, XPointer callData) {
    if (im)
        return;

    im = XOpenIM(display, 0, NULL, NULL);
    if (im) {
        if (!hasUsableInputMethodStyle()) {
            XCloseIM(im);
            im = NULL;
        }
    }

    if (im) {
        XIMCallback callback;
        callback.callback    = (XIMProc)([](XIM im, XPointer clientData, XPointer callData) {
            im = NULL;
        });
        callback.client_data = NULL;
        XSetIMValues(im, XNDestroyCallback, &callback, NULL);

        for (PlatformWindow* window : PlatformWindow::platformWindows)
            createInputContext(window);
    }
}

// Retrieve system content scale via folklore heuristics
//
void getSystemContentScale() {
    // Start by assuming the default X11 DPI
    // NOTE: Some desktop environments (KDE) may remove the Xft.dpi field when it
    //       would be set to 96, so assume that is the case if we cannot find it
    float xdpi = 96.f, ydpi = 96.f;

    // NOTE: Basing the scale on Xft.dpi where available should provide the most
    //       consistent user experience (matches Qt, Gtk, etc), although not
    //       always the most accurate one
    char* rms = XResourceManagerString(display);
    if (rms) {
        XrmDatabase db = XrmGetStringDatabase(rms);
        if (db) {
            SCOPE_EXIT {
                XrmDestroyDatabase(db);
            };
            XrmValue value;
            char* type = NULL;

            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
                if (type && strcmp(type, "String") == 0)
                    xdpi = ydpi = atof(value.addr);
            }
        }
    }
    contentScaleX = xdpi / 96.f;
    contentScaleY = ydpi / 96.f;
    LOG_TRACE(x11, "contentScale = {}x{}", contentScaleX, contentScaleY);
}

// Create the pipe for empty events without assumuing the OS has pipe2(2)
//
bool createEmptyEventPipe() {
    if (pipe(emptyEventPipe) != 0) {
        LOG_ERROR(x11, "Failed to create empty event pipe: {}", strerror(errno));
        return false;
    }

    for (int i = 0; i < 2; i++) {
        const int sf = fcntl(emptyEventPipe[i], F_GETFL, 0);
        const int df = fcntl(emptyEventPipe[i], F_GETFD, 0);

        if (sf == -1 || df == -1 ||                                     //
            fcntl(emptyEventPipe[i], F_SETFL, sf | O_NONBLOCK) == -1 || //
            fcntl(emptyEventPipe[i], F_SETFD, df | FD_CLOEXEC) == -1) {
            LOG_ERROR(x11, "Failed to set flags for empty event pipe: {}", strerror(errno));
            return false;
        }
    }
    return true;
}

// Retrieve a single window property of the specified type
// Inspired by fghGetWindowProperty from freeglut
//
static unsigned long getWindowPropertyX11(Window window, Atom property, Atom type, unsigned char** value) {
    Atom actualType;
    int actualFormat;
    unsigned long itemCount, bytesAfter;

    XGetWindowProperty(display, window, property, 0, LONG_MAX, False, type, &actualType, &actualFormat,
                       &itemCount, &bytesAfter, value);

    return itemCount;
}

// Return the atom ID only if it is listed in the specified array
//
static Atom getAtomIfSupported(Atom* supportedAtoms, unsigned long atomCount, const char* atomName) {
    const Atom atom = XInternAtom(display, atomName, False);

    for (unsigned long i = 0; i < atomCount; i++) {
        if (supportedAtoms[i] == atom)
            return atom;
    }

    return None;
}

static void createKeyTables();

static int initializeX11Depth = 0;

bool initializeX11() {
    if (++initializeX11Depth != 1)
        return true;

    XInitThreads();
    XrmInitialize();

    display = XOpenDisplay(NULL);

    if (!display) {
        const char* name = getenv("DISPLAY");
        if (name) {
            LOG_ERROR(x11, "Failed to open display: {}", name);
        } else {
            LOG_ERROR(x11, "The DISPLAY environment variable is missing");
        }
        return false;
    }

    screen  = DefaultScreen(display);
    root    = RootWindow(display, screen);
    context = XUniqueContext();

    getSystemContentScale();

    if (!createEmptyEventPipe())
        return false;

    NULL_                = XInternAtom(display, "NULL", False);
    UTF8_STRING          = XInternAtom(display, "UTF8_STRING", False);
    ATOM_PAIR            = XInternAtom(display, "ATOM_PAIR", False);

    TARGETS              = XInternAtom(display, "TARGETS", False);
    MULTIPLE             = XInternAtom(display, "MULTIPLE", False);
    PRIMARY              = XInternAtom(display, "PRIMARY", False);
    INCR                 = XInternAtom(display, "INCR", False);
    CLIPBOARD            = XInternAtom(display, "CLIPBOARD", False);

    // Clipboard manager atoms
    CLIPBOARD_MANAGER    = XInternAtom(display, "CLIPBOARD_MANAGER", False);
    SAVE_TARGETS         = XInternAtom(display, "SAVE_TARGETS", False);

    MOTIF_WM_HINTS       = XInternAtom(display, "_MOTIF_WM_HINTS", False);

    NET_SUPPORTED        = XInternAtom(display, "_NET_SUPPORTED", False);

    WM_PROTOCOLS         = XInternAtom(display, "WM_PROTOCOLS", False);
    WM_STATE             = XInternAtom(display, "WM_STATE", False);
    WM_DELETE_WINDOW     = XInternAtom(display, "WM_DELETE_WINDOW", False);
    NET_WM_PING          = XInternAtom(display, "_NET_WM_PING", False);
    NET_WM_PID           = XInternAtom(display, "_NET_WM_PID", False);

    // Xdnd (drag and drop) atoms
    XdndAware            = XInternAtom(display, "XdndAware", False);
    XdndEnter            = XInternAtom(display, "XdndEnter", False);
    XdndPosition         = XInternAtom(display, "XdndPosition", False);
    XdndStatus           = XInternAtom(display, "XdndStatus", False);
    XdndActionCopy       = XInternAtom(display, "XdndActionCopy", False);
    XdndDrop             = XInternAtom(display, "XdndDrop", False);
    XdndFinished         = XInternAtom(display, "XdndFinished", False);
    XdndSelection        = XInternAtom(display, "XdndSelection", False);
    XdndTypeList         = XInternAtom(display, "XdndTypeList", False);
    text_uri_list        = XInternAtom(display, "text/uri-list", False);
    NET_WM_NAME          = XInternAtom(display, "_NET_WM_NAME", False);
    NET_WM_ICON_NAME     = XInternAtom(display, "_NET_WM_ICON_NAME", False);

    Atom* supportedAtoms = NULL;
    const unsigned long atomCount =
        getWindowPropertyX11(root, NET_SUPPORTED, XA_ATOM, (unsigned char**)&supportedAtoms);

    NET_WM_STATE       = getAtomIfSupported(supportedAtoms, atomCount, "_NET_WM_STATE");
    NET_WM_STATE_ABOVE = getAtomIfSupported(supportedAtoms, atomCount, "_NET_WM_STATE_ABOVE");

    NET_WM_STATE_MAXIMIZED_VERT =
        getAtomIfSupported(supportedAtoms, atomCount, "_NET_WM_STATE_MAXIMIZED_VERT");
    NET_WM_STATE_MAXIMIZED_HORZ =
        getAtomIfSupported(supportedAtoms, atomCount, "_NET_WM_STATE_MAXIMIZED_HORZ");

    NET_WM_WINDOW_TYPE        = getAtomIfSupported(supportedAtoms, atomCount, "_NET_WM_WINDOW_TYPE");
    NET_WM_WINDOW_TYPE_NORMAL = getAtomIfSupported(supportedAtoms, atomCount, "_NET_WM_WINDOW_TYPE_NORMAL");
    NET_ACTIVE_WINDOW         = getAtomIfSupported(supportedAtoms, atomCount, "_NET_ACTIVE_WINDOW");

    if (XRRQueryExtension(display, &randr.eventBase, &randr.errorBase)) {
        if (XRRQueryVersion(display, &randr.major, &randr.minor)) {
            // At least version 1.3 required
            if (randr.major > 1 || randr.minor >= 3) {
                // Version ok
            } else {
                LOG_ERROR(x11, "Unsupported RandR version {}.{}", randr.major, randr.minor);
                return false;
            }
        } else {
            LOG_ERROR(x11, "Failed to query RandR version");
            return false;
        }
    } else {
        LOG_ERROR(x11, "Failed to query RandR extension");
        return false;
    }

    XRRScreenResources* sr = XRRGetScreenResourcesCurrent(display, root);
    SCOPE_EXIT {
        XRRFreeScreenResources(sr);
    };
    if (!sr->ncrtc) {
        // A system without CRTCs is likely a system with broken RandR
        // Disable the RandR monitor path and fall back to core functions
        randr.monitorBroken = true;
        LOG_WARN(x11, "RandR is not conformant, disabling");
    }

    if (!randr.monitorBroken) {
        XRRSelectInput(display, root, RROutputChangeNotifyMask);
    }

    xkb.major = 1;
    xkb.minor = 0;
    xkb.available =
        XkbQueryExtension(display, &xkb.majorOpcode, &xkb.eventBase, &xkb.errorBase, &xkb.major, &xkb.minor);

    if (xkb.available) {
        Bool supported;

        if (XkbSetDetectableAutoRepeat(display, True, &supported)) {
            if (supported)
                xkb.detectable = true;
        }

        XkbStateRec state;
        if (XkbGetState(display, XkbUseCoreKbd, &state) == Success)
            xkb.group = (unsigned int)state.group;

        XkbSelectEventDetails(display, XkbUseCoreKbd, XkbStateNotify, XkbGroupStateMask, XkbGroupStateMask);
    } else {
        LOG_WARN(x11, "XKB is not available");
    }

    createKeyTables();

    return true;
}

void terminateX11() {
    if (--initializeX11Depth != 0)
        return;
    XCloseDisplay(display);

    close(emptyEventPipe[0]);
    close(emptyEventPipe[1]);
}

bool initialize() {
    initializeX11();
    if (initialized)
        return true;

    helperWindowHandle = createHelperWindow();

    if (XSupportsLocale()) {
        XSetLocaleModifiers("");

        // If an IM is already present our callback will be called right away
        XRegisterIMInstantiateCallback(display, NULL, NULL, NULL, &inputMethodInstantiateCallback, NULL);
    }

    Internal::updateDisplays();

    initialized = true;
    return true;
}

::Window createHelperWindow() {
    XSetWindowAttributes wa;
    wa.event_mask = PropertyChangeMask;

    return XCreateWindow(display, root, 0, 0, 1, 1, 0, 0, InputOnly, DefaultVisual(display, screen),
                         CWEventMask, &wa);
}

void terminate() {
    if (!initialized)
        return;
    if (helperWindowHandle) {
        if (XGetSelectionOwner(display, CLIPBOARD) == helperWindowHandle) {
            pushSelectionToManagerX11();
        }

        XDestroyWindow(display, helperWindowHandle);
        helperWindowHandle = None;
    }

    terminateX11();
    initialized = false;
}

// Set the specified property to the selection converted to the requested target
//
Atom writeTargetToProperty(const XSelectionRequestEvent* request) {
    char* selectionString = NULL;
    const Atom formats[]  = { UTF8_STRING, XA_STRING };
    const int formatCount = sizeof(formats) / sizeof(formats[0]);

    if (request->selection == PRIMARY)
        selectionString = primarySelectionString;
    else
        selectionString = clipboardString;

    if (request->property == None) {
        // The requester is a legacy client (ICCCM section 2.2)
        // We don't support legacy clients, so fail here
        return None;
    }

    if (request->target == TARGETS) {
        // The list of supported targets was requested

        const Atom targets[] = { TARGETS, MULTIPLE, UTF8_STRING, XA_STRING };

        XChangeProperty(display, request->requestor, request->property, XA_ATOM, 32, PropModeReplace,
                        (unsigned char*)targets, sizeof(targets) / sizeof(targets[0]));

        return request->property;
    }

    if (request->target == MULTIPLE) {
        // Multiple conversions were requested

        Atom* targets;
        const unsigned long count =
            getWindowPropertyX11(request->requestor, request->property, ATOM_PAIR, (unsigned char**)&targets);

        for (unsigned long i = 0; i < count; i += 2) {
            int j;

            for (j = 0; j < formatCount; j++) {
                if (targets[i] == formats[j])
                    break;
            }

            if (j < formatCount) {
                XChangeProperty(display, request->requestor, targets[i + 1], targets[i], 8, PropModeReplace,
                                (unsigned char*)selectionString, strlen(selectionString));
            } else
                targets[i + 1] = None;
        }

        XChangeProperty(display, request->requestor, request->property, ATOM_PAIR, 32, PropModeReplace,
                        (unsigned char*)targets, count);

        XFree(targets);

        return request->property;
    }

    if (request->target == SAVE_TARGETS) {
        // The request is a check whether we support SAVE_TARGETS
        // It should be handled as a no-op side effect target

        XChangeProperty(display, request->requestor, request->property, NULL_, 32, PropModeReplace, NULL, 0);

        return request->property;
    }

    // Conversion to a data target was requested

    for (int i = 0; i < formatCount; i++) {
        if (request->target == formats[i]) {
            // The requested target is one we support

            XChangeProperty(display, request->requestor, request->property, request->target, 8,
                            PropModeReplace, (unsigned char*)selectionString, strlen(selectionString));

            return request->property;
        }
    }

    // The requested target is not supported

    return None;
}

void handleSelectionRequest(XEvent* event) {
    const XSelectionRequestEvent* request = &event->xselectionrequest;

    XEvent reply                          = { SelectionNotify };
    reply.xselection.property             = writeTargetToProperty(request);
    reply.xselection.display              = request->display;
    reply.xselection.requestor            = request->requestor;
    reply.xselection.selection            = request->selection;
    reply.xselection.target               = request->target;
    reply.xselection.time                 = request->time;

    XSendEvent(display, request->requestor, False, 0, &reply);
}

// Returns whether the event is a selection event
//
static Bool isSelectionEvent(Display* display, XEvent* event, XPointer pointer) {
    if (event->xany.window != helperWindowHandle)
        return False;

    return event->type == SelectionRequest || event->type == SelectionNotify || event->type == SelectionClear;
}

void pushSelectionToManagerX11() {
    XConvertSelection(display, CLIPBOARD_MANAGER, SAVE_TARGETS, None, helperWindowHandle, CurrentTime);

    for (;;) {
        XEvent event;

        while (XCheckIfEvent(display, &event, &isSelectionEvent, NULL)) {
            switch (event.type) {
            case SelectionRequest:
                handleSelectionRequest(&event);
                break;

            case SelectionNotify: {
                if (event.xselection.target == SAVE_TARGETS) {
                    // This means one of two things; either the selection
                    // was not owned, which means there is no clipboard
                    // manager, or the transfer to the clipboard manager has
                    // completed
                    // In either case, it means we are done here
                    return;
                }

                break;
            }
            }
        }

        waitForX11Event(NULL);
    }
}

bool pollPOSIX(struct pollfd* fds, nfds_t count, double* timeout) {
    for (;;) {
        if (timeout) {
            const double base        = currentTime();

            const time_t seconds     = (time_t)*timeout;
            const long nanoseconds   = (long)((*timeout - seconds) * 1e9);
            const struct timespec ts = { seconds, nanoseconds };
            const int result         = ppoll(fds, count, &ts, NULL);
            const int error          = errno; // clock_gettime may overwrite our error

            *timeout -= (currentTime() - base);

            if (result > 0)
                return true;
            else if (result == -1 && error != EINTR && error != EAGAIN)
                return false;
            else if (*timeout <= 0.0)
                return false;
        } else {
            const int result = poll(fds, count, -1);
            if (result > 0)
                return true;
            else if (result == -1 && errno != EINTR && errno != EAGAIN)
                return false;
        }
    }
}

// Wait for event data to arrive on the X11 display socket
// This avoids blocking other threads via the per-display Xlib lock that also
// covers GLX functions
//
static bool waitForX11Event(double* timeout) {
    struct pollfd fd = { ConnectionNumber(display), POLLIN };

    while (!XPending(display)) {
        if (!pollPOSIX(&fd, 1, timeout))
            return false;
    }

    return true;
}

// Wait for event data to arrive on any event file descriptor
// This avoids blocking other threads via the per-display Xlib lock that also
// covers GLX functions
//
static bool waitForAnyEvent(double* timeout) {
    enum { XLIB_FD, PIPE_FD, INOTIFY_FD };
    struct pollfd fds[] = { /* [XLIB_FD]    = */ { ConnectionNumber(display), POLLIN },
                            /* [PIPE_FD]    = */ { emptyEventPipe[0], POLLIN },
                            /* [INOTIFY_FD] = */ { -1, POLLIN } };

    while (!XPending(display)) {
        if (!pollPOSIX(fds, sizeof(fds) / sizeof(fds[0]), timeout))
            return false;

        for (int i = 1; i < sizeof(fds) / sizeof(fds[0]); i++) {
            if (fds[i].revents & POLLIN)
                return true;
        }
    }

    return true;
}

// Writes a byte to the empty event pipe
//
static void writeEmptyEvent(void) {
    for (;;) {
        const char byte      = 0;
        const ssize_t result = write(emptyEventPipe[1], &byte, 1);
        if (result == 1 || (result == -1 && errno != EINTR))
            break;
    }
}

static void setStyleDecorated(PlatformWindowData* m_data, bool enabled) {
    // Motif WM hints flags
    const unsigned long MWM_HINTS_DECORATIONS = 2;
    const unsigned long MWM_DECOR_ALL         = 1;

    struct {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long input_mode;
        unsigned long status;
    } hints           = { 0 };

    hints.flags       = MWM_HINTS_DECORATIONS;
    hints.decorations = enabled ? MWM_DECOR_ALL : 0;

    XChangeProperty(display, m_data->handle, MOTIF_WM_HINTS, MOTIF_WM_HINTS, 32, PropModeReplace,
                    (unsigned char*)&hints, sizeof(hints) / sizeof(long));
}

static int errorCode = Success;
static XErrorHandler errorHandler;

// X error handler
//
static int errorHandlerFunc(Display* display, XErrorEvent* event) {
    if (X11::display != display)
        return 0;

    errorCode = event->error_code;
    return 0;
}

static void grabErrorHandlerX11() {
    assert(errorHandler == NULL);
    errorCode    = Success;
    errorHandler = XSetErrorHandler(&errorHandlerFunc);
}

static void releaseErrorHandlerX11() {
    // Synchronize to make sure all commands are processed
    XSync(display, False);
    XSetErrorHandler(errorHandler);
    errorHandler = NULL;
}

static void setWindowTitle(PlatformWindowData* m_data, const std::string& title) {
    Xutf8SetWMProperties(display, m_data->handle, title.c_str(), title.c_str(), NULL, 0, NULL, NULL, NULL);

    XChangeProperty(X11::display, m_data->handle, X11::NET_WM_NAME, X11::UTF8_STRING, 8, PropModeReplace,
                    (unsigned char*)title.c_str(), title.size());

    XChangeProperty(X11::display, m_data->handle, X11::NET_WM_ICON_NAME, X11::UTF8_STRING, 8, PropModeReplace,
                    (unsigned char*)title.c_str(), title.size());

    XFlush(X11::display);
}

// Updates the normal hints according to the window settings
//
static void updateNormalHints(PlatformWindow* window, Size size) {
    XSizeHints* hints = XAllocSizeHints();

    long supplied;
    XGetWMNormalHints(display, window->m_data->handle, hints, &supplied);

    hints->flags &= ~(PMinSize | PMaxSize | PAspect);

    if (window->m_windowStyle && WindowStyle::Resizable) {
        if (window->m_minSize != Size{ PlatformWindow::dontCare, PlatformWindow::dontCare }) {
            hints->flags |= PMinSize;
            hints->min_width  = window->m_minSize.width;
            hints->min_height = window->m_minSize.height;
        }

        if (window->m_maxSize != Size{ PlatformWindow::dontCare, PlatformWindow::dontCare }) {
            hints->flags |= PMaxSize;
            hints->max_width  = window->m_maxSize.width;
            hints->max_height = window->m_maxSize.height;
        }
    } else {
        hints->flags |= (PMinSize | PMaxSize);
        hints->min_width = hints->max_width = size.width;
        hints->min_height = hints->max_height = size.height;
    }
    XSetWMNormalHints(display, window->m_data->handle, hints);
    XFree(hints);
}

static Size getWindowSize(const PlatformWindowData* m_data) {
    XWindowAttributes attribs;
    XGetWindowAttributes(display, m_data->handle, &attribs);
    return { attribs.width, attribs.height };
}

static Point getWindowPos(const PlatformWindowData* m_data) {
    Window dummy;
    Point result{ 0, 0 };

    XTranslateCoordinates(display, m_data->handle, root, 0, 0, &result.x, &result.y, &dummy);
    return result;
}

static void setWindowSize(PlatformWindow* window, Size size) {
    if (!(window->m_windowStyle && WindowStyle::Resizable))
        updateNormalHints(window, size);

    XResizeWindow(display, window->m_data->handle, size.width, size.height);

    XFlush(display);
}

// Returns whether the window is iconified
//
static int getWindowState(const PlatformWindow* window) {
    int result = WithdrawnState;

    struct {
        uint32_t state;
        Window icon;
    }* state = NULL;

    if (getWindowPropertyX11(window->m_data->handle, WM_STATE, WM_STATE, (unsigned char**)&state) >= 2) {
        result = state->state;
    }

    if (state)
        XFree(state);

    return result;
}

static bool windowIconified(const PlatformWindow* window) {
    return getWindowState(window) == IconicState;
}

static bool windowFocused(const PlatformWindow* window) {
    Window focused;
    int state;

    XGetInputFocus(display, &focused, &state);
    return window->m_data->handle == focused;
}

static bool windowVisible(const PlatformWindow* window) {
    XWindowAttributes wa;
    XGetWindowAttributes(display, window->m_data->handle, &wa);
    return wa.map_state == IsViewable;
}

static bool windowHovered(const PlatformWindow* window) {
    Window w = root;
    while (w) {
        Window root;
        int rootX, rootY, childX, childY;
        unsigned int mask;

        grabErrorHandlerX11();

        const Bool result = XQueryPointer(display, w, &root, &w, &rootX, &rootY, &childX, &childY, &mask);

        releaseErrorHandlerX11();

        if (errorCode == BadWindow)
            w = root;
        else if (!result)
            return false;
        else if (w == window->m_data->handle)
            return true;
    }

    return false;
}

static bool windowMaximized(const PlatformWindow* window) {
    Atom* states;
    bool maximized = false;

    if (!NET_WM_STATE || !NET_WM_STATE_MAXIMIZED_VERT || !NET_WM_STATE_MAXIMIZED_HORZ) {
        return maximized;
    }

    const unsigned long count =
        getWindowPropertyX11(window->m_data->handle, NET_WM_STATE, XA_ATOM, (unsigned char**)&states);
    SCOPE_EXIT {
        if (states)
            XFree(states);
    };

    for (unsigned long i = 0; i < count; i++) {
        if (states[i] == NET_WM_STATE_MAXIMIZED_VERT || states[i] == NET_WM_STATE_MAXIMIZED_HORZ) {
            maximized = true;
            break;
        }
    }

    return maximized;
}

// Waits until a VisibilityNotify event arrives for the specified window or the
// timeout period elapses (ICCCM section 4.2.2)
//
static bool waitForVisibilityNotify(const PlatformWindow* window) {
    XEvent dummy;
    double timeout = 0.1;

    while (!XCheckTypedWindowEvent(display, window->m_data->handle, VisibilityNotify, &dummy)) {
        if (!waitForX11Event(&timeout))
            return false;
    }

    return true;
}

// Sends an EWMH or ICCCM event to the window manager
//
static void sendEventToWM(const PlatformWindow* window, Atom type, long a, long b, long c, long d, long e) {
    XEvent event               = { ClientMessage };
    event.xclient.window       = window->m_data->handle;
    event.xclient.format       = 32; // Data is 32-bit longs
    event.xclient.message_type = type;
    event.xclient.data.l[0]    = a;
    event.xclient.data.l[1]    = b;
    event.xclient.data.l[2]    = c;
    event.xclient.data.l[3]    = d;
    event.xclient.data.l[4]    = e;

    XSendEvent(display, root, False, SubstructureNotifyMask | SubstructureRedirectMask, &event);
}

static void showWindow(const PlatformWindow* window) {
    if (windowVisible(window))
        return;

    XMapWindow(display, window->m_data->handle);
    waitForVisibilityNotify(window);
}

static void focusWindow(const PlatformWindow* window) {
    if (NET_ACTIVE_WINDOW)
        sendEventToWM(window, NET_ACTIVE_WINDOW, 1, 0, 0, 0, 0);
    else if (windowVisible(window)) {
        XRaiseWindow(display, window->m_data->handle);
        XSetInputFocus(display, window->m_data->handle, RevertToParent, CurrentTime);
    }

    XFlush(display);
}

static void hideWindow(const PlatformWindow* window) {
    XUnmapWindow(display, window->m_data->handle);
    XFlush(display);
}

static void setWindowPos(PlatformWindow* window, Point position) {
    // HACK: Explicitly setting PPosition to any value causes some WMs, notably
    //       Compiz and Metacity, to honor the position of unmapped windows
    if (!windowVisible(window)) {
        long supplied;
        XSizeHints* hints = XAllocSizeHints();

        if (XGetWMNormalHints(display, window->m_data->handle, hints, &supplied)) {
            hints->flags |= PPosition;
            hints->x = hints->y = 0;

            XSetWMNormalHints(display, window->m_data->handle, hints);
        }

        XFree(hints);
    }

    XMoveWindow(display, window->m_data->handle, position.x, position.y);
    XFlush(display);
}

// Clear its handle when the input context has been destroyed
//
static void inputContextDestroyCallback(XIC ic, XPointer clientData, XPointer callData) {
    PlatformWindow* window = (PlatformWindow*)clientData;
    window->m_data->ic     = NULL;
}

static void createInputContext(PlatformWindow* window) {
    XIMCallback callback;
    callback.callback    = (XIMProc)inputContextDestroyCallback;
    callback.client_data = (XPointer)window;

    window->m_data->ic   = XCreateIC(im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow,
                                     window->m_data->handle, XNFocusWindow, window->m_data->handle,
                                     XNDestroyCallback, &callback, NULL);

    if (window->m_data->ic) {
        XWindowAttributes attribs;
        XGetWindowAttributes(display, window->m_data->handle, &attribs);

        unsigned long filter = 0;
        if (XGetICValues(window->m_data->ic, XNFilterEvents, &filter, NULL) == NULL) {
            XSelectInput(display, window->m_data->handle, attribs.your_event_mask | filter);
        }
    }
}

// Drains available data from the empty event pipe
//
static void drainEmptyEvents() {
    for (;;) {
        char dummy[64];
        const ssize_t result = read(emptyEventPipe[0], dummy, sizeof(dummy));
        if (result == -1 && errno != EINTR)
            break;
    }
}

// Translates an X11 key code to a key token
//
static KeyCode translateKey(int scancode) {
    // Use the pre-filled LUT (see createKeyTables() in x11_init.c)
    if (scancode < 0 || scancode > 255)
        return KeyCode::Unknown;

    return scanCodeToKeyCodeTable[scancode];
}

// Translates an X event modifier state mask
//
static KeyModifiers translateState(int state) {
    KeyModifiers mods = KeyModifiers::None;

    if (state & ShiftMask)
        mods |= KeyModifiers::Shift;
    if (state & ControlMask)
        mods |= KeyModifiers::Control;
    if (state & Mod1Mask)
        mods |= KeyModifiers::Alt;
    if (state & Mod4Mask)
        mods |= KeyModifiers::Super;
    if (state & LockMask)
        mods |= KeyModifiers::CapsLock;
    if (state & Mod2Mask)
        mods |= KeyModifiers::NumLock;

    return mods;
}

static void setStyleTopMost(PlatformWindow* window, bool enabled) {
    if (!NET_WM_STATE || !NET_WM_STATE_ABOVE)
        return;

    if (windowVisible(window)) {
        const long action = enabled ? NET_WM_STATE_ADD_ : NET_WM_STATE_REMOVE_;
        sendEventToWM(window, NET_WM_STATE, action, NET_WM_STATE_ABOVE, 0, 1, 0);
    } else {
        Atom* states = NULL;
        const unsigned long count =
            getWindowPropertyX11(window->m_data->handle, NET_WM_STATE, XA_ATOM, (unsigned char**)&states);

        // NOTE: We don't check for failure as this property may not exist yet
        //       and that's fine (and we'll create it implicitly with append)

        if (enabled) {
            unsigned long i;

            for (i = 0; i < count; i++) {
                if (states[i] == NET_WM_STATE_ABOVE)
                    break;
            }

            if (i == count) {
                XChangeProperty(display, window->m_data->handle, NET_WM_STATE, XA_ATOM, 32, PropModeAppend,
                                (unsigned char*)&NET_WM_STATE_ABOVE, 1);
            }
        } else if (states) {
            for (unsigned long i = 0; i < count; i++) {
                if (states[i] == NET_WM_STATE_ABOVE) {
                    states[i] = states[count - 1];
                    XChangeProperty(display, window->m_data->handle, NET_WM_STATE, XA_ATOM, 32,
                                    PropModeReplace, (unsigned char*)states, count - 1);
                    break;
                }
            }
        }

        if (states)
            XFree(states);
    }

    XFlush(display);
}

char32_t keySym2Unicode(KeySym ks);

// Process the specified X event
//
static void processEvent(XEvent* event) {
    int keycode   = 0;
    Bool filtered = False;

    // HACK: Save scancode as some IMs clear the field in XFilterEvent
    if (event->type == KeyPress || event->type == KeyRelease)
        keycode = event->xkey.keycode;

    filtered = XFilterEvent(event, None);

    if (event->type == randr.eventBase + RRNotify) {
        XRRUpdateConfiguration(event);
        Internal::updateDisplays();
        return;
    }

    if (event->type == xkb.eventBase + XkbEventCode) {
        if (((XkbEvent*)event)->any.xkb_type == XkbStateNotify &&
            (((XkbEvent*)event)->state.changed & XkbGroupStateMask)) {
            xkb.group = ((XkbEvent*)event)->state.group;
        }

        return;
    }

    if (event->type == SelectionRequest) {
        handleSelectionRequest(event);
        return;
    }

    PlatformWindow* window = NULL;
    if (XFindContext(display, event->xany.window, context, (XPointer*)&window) != 0) {
        // This is an event for a window that has already been destroyed
        return;
    }

    switch (event->type) {
    case ReparentNotify: {
        window->m_data->parent = event->xreparent.parent;
        return;
    }

    case KeyPress: {
        const KeyCode key       = translateKey(keycode);
        const KeyModifiers mods = translateState(event->xkey.state);
        const int plain         = (mods & (KeyModifiers::Control | KeyModifiers::Alt)) == KeyModifiers::None;

        if (window->m_data->ic) {
            // HACK: Do not report the key press events duplicated by XIM
            //       Duplicate key releases are filtered out implicitly by
            //       the key repeat logic in InputKey
            //       A timestamp per key is used to handle simultaneous keys
            // NOTE: Always allow the first event for each key through
            //       (the server never sends a timestamp of zero)
            // NOTE: Timestamp difference is compared to handle wrap-around
            Time diff = event->xkey.time - window->m_data->keyPressTimes[keycode];
            if (diff == event->xkey.time || (diff > 0 && diff < ((Time)1 << 31))) {
                if (keycode)
                    window->keyEvent(key, keycode, KeyAction::Press, mods);

                window->m_data->keyPressTimes[keycode] = event->xkey.time;
            }

            if (!filtered) {
                int count;
                Status status;
                std::string buffer(100, ' ');

                count = Xutf8LookupString(window->m_data->ic, &event->xkey, buffer.data(), buffer.size() - 1,
                                          NULL, &status);

                if (status == XBufferOverflow) {
                    buffer.resize(count + 1);
                    count = Xutf8LookupString(window->m_data->ic, &event->xkey, buffer.data(),
                                              buffer.size() - 1, NULL, &status);
                }

                if (status == XLookupChars || status == XLookupBoth) {
                    buffer.resize(count);
                    std::u32string utf32 = utf8ToUtf32(buffer);
                    for (char32_t c : utf32) {
                        window->charEvent(c, false);
                    }
                }
            }
        } else {
            KeySym keysym;
            XLookupString(&event->xkey, NULL, 0, &keysym, NULL);

            window->keyEvent(key, keycode, KeyAction::Press, mods);

            const char32_t codepoint = keySym2Unicode(keysym);
            if (codepoint != char32_t(-1))
                window->charEvent(codepoint, false);
        }

        return;
    }

    case KeyRelease: {
        const KeyCode key       = translateKey(keycode);
        const KeyModifiers mods = translateState(event->xkey.state);

        if (!xkb.detectable) {
            // HACK: Key repeat events will arrive as KeyRelease/KeyPress
            //       pairs with similar or identical time stamps
            //       The key repeat logic in InputKey expects only key
            //       presses to repeat, so detect and discard release events
            if (XEventsQueued(display, QueuedAfterReading)) {
                XEvent next;
                XPeekEvent(display, &next);

                if (next.type == KeyPress && next.xkey.window == event->xkey.window &&
                    next.xkey.keycode == keycode) {
                    // HACK: The time of repeat events sometimes doesn't
                    //       match that of the press event, so add an
                    //       epsilon
                    //       Toshiyuki Takahashi can press a button
                    //       16 times per second so it's fairly safe to
                    //       assume that no human is pressing the key 50
                    //       times per second (value is ms)
                    if ((next.xkey.time - event->xkey.time) < 20) {
                        // This is very likely a server-generated key repeat
                        // event, so ignore it
                        return;
                    }
                }
            }
        }

        window->keyEvent(key, keycode, KeyAction::Release, mods);
        return;
    }

    case ButtonPress: {
        const KeyModifiers mods = translateState(event->xbutton.state);

        if (event->xbutton.button == Button1)
            window->mouseEvent(MouseButton::Left, MouseAction::Press, mods, window->m_data->lastCursorPos);
        else if (event->xbutton.button == Button2)
            window->mouseEvent(MouseButton::Middle, MouseAction::Press, mods, window->m_data->lastCursorPos);
        else if (event->xbutton.button == Button3)
            window->mouseEvent(MouseButton::Right, MouseAction::Press, mods, window->m_data->lastCursorPos);

        // Modern X provides scroll events as mouse button presses
        else if (event->xbutton.button == Button4)
            window->wheelEvent(0.0, 1.0);
        else if (event->xbutton.button == Button5)
            window->wheelEvent(0.0, -1.0);
        else if (event->xbutton.button == Button5 + 1)
            window->wheelEvent(1.0, 0.0);
        else if (event->xbutton.button == Button5 + 2)
            window->wheelEvent(-1.0, 0.0);

        else {
            // Additional buttons after 7 are treated as regular buttons
            // We subtract 4 to fill the gap left by scroll input above
            window->mouseEvent(static_cast<MouseButton>(event->xbutton.button - Button1 - 4),
                               MouseAction::Press, mods, window->m_data->lastCursorPos);
        }

        return;
    }

    case ButtonRelease: {
        const KeyModifiers mods = translateState(event->xbutton.state);

        if (event->xbutton.button == Button1)
            window->mouseEvent(MouseButton::Left, MouseAction::Release, mods, window->m_data->lastCursorPos);
        else if (event->xbutton.button == Button2)
            window->mouseEvent(MouseButton::Middle, MouseAction::Release, mods,
                               window->m_data->lastCursorPos);
        else if (event->xbutton.button == Button3)
            window->mouseEvent(MouseButton::Right, MouseAction::Release, mods, window->m_data->lastCursorPos);
        else if (event->xbutton.button > Button5 + 2) {
            // Additional buttons after 7 are treated as regular buttons
            // We subtract 4 to fill the gap left by scroll input above
            window->mouseEvent(static_cast<MouseButton>(event->xbutton.button - Button1 - 4),
                               MouseAction::Release, mods, window->m_data->lastCursorPos);
        }

        return;
    }

    case EnterNotify: {
        // XEnterWindowEvent is XCrossingEvent
        const int x = event->xcrossing.x;
        const int y = event->xcrossing.y;

        window->mouseEnterOrLeave(true);
        window->mouseMove(Point(x, y));

        window->m_data->lastCursorPos.x = x;
        window->m_data->lastCursorPos.y = y;
        return;
    }

    case LeaveNotify: {
        window->mouseEnterOrLeave(false);
        return;
    }

    case MotionNotify: {
        const int x = event->xmotion.x;
        const int y = event->xmotion.y;

        if (x != window->m_data->warpCursorPos.x || y != window->m_data->warpCursorPos.y) {
            // The cursor was moved by something other

            window->mouseMove(Point{ x, y });
        }

        window->m_data->lastCursorPos.x = x;
        window->m_data->lastCursorPos.y = y;
        return;
    }

    case ConfigureNotify: {
        if (event->xconfigure.width != window->m_data->width ||
            event->xconfigure.height != window->m_data->height) {
            window->m_data->width     = event->xconfigure.width;
            window->m_data->height    = event->xconfigure.height;

            window->m_windowSize      = { window->m_data->width, window->m_data->height };
            window->m_framebufferSize = window->m_windowSize;

            window->windowResized(window->m_windowSize, window->m_framebufferSize);
        }

        int xpos = event->xconfigure.x;
        int ypos = event->xconfigure.y;

        // NOTE: ConfigureNotify events from the server are in local
        //       coordinates, so if we are reparented we need to translate
        //       the position into root (screen) coordinates
        if (!event->xany.send_event && window->m_data->parent != root) {
            grabErrorHandlerX11();

            Window dummy;
            XTranslateCoordinates(display, window->m_data->parent, root, xpos, ypos, &xpos, &ypos, &dummy);

            releaseErrorHandlerX11();
            if (errorCode == BadWindow)
                return;
        }

        if (xpos != window->m_data->xpos || ypos != window->m_data->ypos) {
            window->m_data->xpos = xpos;
            window->m_data->ypos = ypos;

            window->windowMoved({ xpos, ypos });
        }

        return;
    }

    case ClientMessage: {
        // Custom client message, probably from the window manager

        if (filtered)
            return;

        if (event->xclient.message_type == None)
            return;

        if (event->xclient.message_type == WM_PROTOCOLS) {
            const Atom protocol = event->xclient.data.l[0];
            if (protocol == None)
                return;

            if (protocol == WM_DELETE_WINDOW) {
                // The window manager was asked to close the window, for
                // example by the user pressing a 'close' window decoration
                // button
                window->closeAttempt();
            } else if (protocol == NET_WM_PING) {
                // The window manager is pinging the application to ensure
                // it's still responding to events

                XEvent reply         = *event;
                reply.xclient.window = root;

                XSendEvent(display, root, False, SubstructureNotifyMask | SubstructureRedirectMask, &reply);
            }
        } else if (event->xclient.message_type == XdndEnter) {
            // A drag operation has entered the window
            unsigned long count;
            Atom* formats   = NULL;
            const bool list = event->xclient.data.l[1] & 1;

            xdnd.source     = event->xclient.data.l[0];
            xdnd.version    = event->xclient.data.l[1] >> 24;
            xdnd.format     = None;

            if (xdnd.version > XDND_VERSION)
                return;

            if (list) {
                count = getWindowPropertyX11(xdnd.source, XdndTypeList, XA_ATOM, (unsigned char**)&formats);
            } else {
                count   = 3;
                formats = (Atom*)event->xclient.data.l + 2;
            }

            for (unsigned int i = 0; i < count; i++) {
                if (formats[i] == text_uri_list) {
                    xdnd.format = text_uri_list;
                    break;
                }
            }

            if (list && formats)
                XFree(formats);
        } else if (event->xclient.message_type == XdndDrop) {
            // The drag operation has finished by dropping on the window
            Time time = CurrentTime;

            if (xdnd.version > XDND_VERSION)
                return;

            if (xdnd.format) {
                if (xdnd.version >= 1)
                    time = event->xclient.data.l[2];

                // Request the chosen format from the source window
                XConvertSelection(display, XdndSelection, xdnd.format, XdndSelection, window->m_data->handle,
                                  time);
            } else if (xdnd.version >= 2) {
                XEvent reply               = { ClientMessage };
                reply.xclient.window       = xdnd.source;
                reply.xclient.message_type = XdndFinished;
                reply.xclient.format       = 32;
                reply.xclient.data.l[0]    = window->m_data->handle;
                reply.xclient.data.l[1]    = 0; // The drag was rejected
                reply.xclient.data.l[2]    = None;

                XSendEvent(display, xdnd.source, False, NoEventMask, &reply);
                XFlush(display);
            }
        } else if (event->xclient.message_type == XdndPosition) {
            // The drag operation has moved over the window
            const int xabs = (event->xclient.data.l[2] >> 16) & 0xffff;
            const int yabs = (event->xclient.data.l[2]) & 0xffff;
            Window dummy;
            int xpos, ypos;

            if (xdnd.version > XDND_VERSION)
                return;

            XTranslateCoordinates(display, root, window->m_data->handle, xabs, yabs, &xpos, &ypos, &dummy);

            window->mouseMove(Point(xpos, ypos));

            XEvent reply               = { ClientMessage };
            reply.xclient.window       = xdnd.source;
            reply.xclient.message_type = XdndStatus;
            reply.xclient.format       = 32;
            reply.xclient.data.l[0]    = window->m_data->handle;
            reply.xclient.data.l[2]    = 0; // Specify an empty rectangle
            reply.xclient.data.l[3]    = 0;

            if (xdnd.format) {
                // Reply that we are ready to copy the dragged data
                reply.xclient.data.l[1] = 1; // Accept with no rectangle
                if (xdnd.version >= 2)
                    reply.xclient.data.l[4] = XdndActionCopy;
            }

            XSendEvent(display, xdnd.source, False, NoEventMask, &reply);
            XFlush(display);
        }

        return;
    }

    case SelectionNotify: {
        if (event->xselection.property == XdndSelection) {
            // The converted data from the drag operation has arrived
            char* data;
            const unsigned long result =
                getWindowPropertyX11(event->xselection.requestor, event->xselection.property,
                                     event->xselection.target, (unsigned char**)&data);
            SCOPE_EXIT {
                if (data)
                    XFree(data);
            };

            if (result) {
                std::vector<std::string_view> uriList = split(data, "\r\n");
                for (std::string_view& uri : uriList) {
                    uri = uri.substr(uri.find_first_not_of(" "));
                    if (uri.starts_with("#") || !(uri.starts_with("file://"))) {
                        uri = {};
                    }
                    uri = uri.substr(0, uri.find_first_of(" "));
                }
                window->filesDropped(toStrings(uriList));
            }

            if (xdnd.version >= 2) {
                XEvent reply               = { ClientMessage };
                reply.xclient.window       = xdnd.source;
                reply.xclient.message_type = XdndFinished;
                reply.xclient.format       = 32;
                reply.xclient.data.l[0]    = window->m_data->handle;
                reply.xclient.data.l[1]    = result;
                reply.xclient.data.l[2]    = XdndActionCopy;

                XSendEvent(display, xdnd.source, False, NoEventMask, &reply);
                XFlush(display);
            }
        }

        return;
    }

    case FocusIn: {
        if (event->xfocus.mode == NotifyGrab || event->xfocus.mode == NotifyUngrab) {
            // Ignore focus events from popup indicator windows, window menu
            // key chords and window dragging
            return;
        }

        if (window->m_data->ic)
            XSetICFocus(window->m_data->ic);

        window->focusChange(true);
        return;
    }

    case FocusOut: {
        if (event->xfocus.mode == NotifyGrab || event->xfocus.mode == NotifyUngrab) {
            // Ignore focus events from popup indicator windows, window menu
            // key chords and window dragging
            return;
        }

        if (window->m_data->ic)
            XUnsetICFocus(window->m_data->ic);

        window->focusChange(false);
        return;
    }

    case Expose: {
        return;
    }

    case PropertyNotify: {
        if (event->xproperty.state != PropertyNewValue)
            return;

        if (event->xproperty.atom == WM_STATE) {
            const int state = getWindowState(window);
            if (state != IconicState && state != NormalState)
                return;

            const bool iconified = (state == IconicState);
            if (window->m_data->iconified != iconified) {
                window->windowStateChanged(iconified, window->m_maximized);
                window->m_data->iconified = iconified;
            }
        } else if (event->xproperty.atom == NET_WM_STATE) {
            const bool maximized = windowMaximized(window);
            if (window->m_data->maximized != maximized) {
                window->m_data->maximized = maximized;
                window->windowStateChanged(window->m_data->iconified, maximized);
            }
        }

        return;
    }

    case DestroyNotify:
        return;
    }
}

static void pollEvents() {
    drainEmptyEvents();

    XPending(display);

    while (QLength(display)) {
        XEvent event;
        XNextEvent(display, &event);
        processEvent(&event);
    }

    XFlush(display);
}

void iconifyWindow(PlatformWindow* window) {
    XIconifyWindow(display, window->m_data->handle, screen);
    XFlush(display);
}

void restoreWindow(PlatformWindow* window) {
    if (windowIconified(window)) {
        XMapWindow(display, window->m_data->handle);
        waitForVisibilityNotify(window);
    } else if (windowVisible(window)) {
        if (NET_WM_STATE && NET_WM_STATE_MAXIMIZED_VERT && NET_WM_STATE_MAXIMIZED_HORZ) {
            sendEventToWM(window, NET_WM_STATE, NET_WM_STATE_REMOVE_, NET_WM_STATE_MAXIMIZED_VERT,
                          NET_WM_STATE_MAXIMIZED_HORZ, 1, 0);
        }
    }

    XFlush(display);
}

void maximizeWindow(PlatformWindow* window) {
    if (!NET_WM_STATE || !NET_WM_STATE_MAXIMIZED_VERT || !NET_WM_STATE_MAXIMIZED_HORZ) {
        return;
    }

    if (X11::windowVisible(window)) {
        sendEventToWM(window, NET_WM_STATE, NET_WM_STATE_ADD_, NET_WM_STATE_MAXIMIZED_VERT,
                      NET_WM_STATE_MAXIMIZED_HORZ, 1, 0);
    } else {
        Atom* states = NULL;
        unsigned long count =
            getWindowPropertyX11(window->m_data->handle, NET_WM_STATE, XA_ATOM, (unsigned char**)&states);

        // NOTE: We don't check for failure as this property may not exist yet
        //       and that's fine (and we'll create it implicitly with append)

        Atom missing[2]            = { NET_WM_STATE_MAXIMIZED_VERT, NET_WM_STATE_MAXIMIZED_HORZ };
        unsigned long missingCount = 2;

        for (unsigned long i = 0; i < count; i++) {
            for (unsigned long j = 0; j < missingCount; j++) {
                if (states[i] == missing[j]) {
                    missing[j] = missing[missingCount - 1];
                    missingCount--;
                }
            }
        }

        if (states)
            XFree(states);

        if (!missingCount)
            return;

        XChangeProperty(display, window->m_data->handle, NET_WM_STATE, XA_ATOM, 32, PropModeAppend,
                        (unsigned char*)missing, missingCount);
    }

    XFlush(display);
}

// Translate the X11 KeySyms for a key to a key code
// NOTE: This is only used as a fallback, in case the XKB method fails
//       It is layout-dependent and will fail partially on most non-US layouts
//
static KeyCode translateKeySyms(const KeySym* keysyms, int width) {
    if (width > 1) {
        switch (keysyms[1]) {
        case XK_KP_0:
            return KeyCode::KP0;
        case XK_KP_1:
            return KeyCode::KP1;
        case XK_KP_2:
            return KeyCode::KP2;
        case XK_KP_3:
            return KeyCode::KP3;
        case XK_KP_4:
            return KeyCode::KP4;
        case XK_KP_5:
            return KeyCode::KP5;
        case XK_KP_6:
            return KeyCode::KP6;
        case XK_KP_7:
            return KeyCode::KP7;
        case XK_KP_8:
            return KeyCode::KP8;
        case XK_KP_9:
            return KeyCode::KP9;
        case XK_KP_Separator:
        case XK_KP_Decimal:
            return KeyCode::KPDecimal;
        case XK_KP_Equal:
            return KeyCode::KPEqual;
        case XK_KP_Enter:
            return KeyCode::KPEnter;
        default:
            break;
        }
    }

    switch (keysyms[0]) {
    case XK_Escape:
        return KeyCode::Escape;
    case XK_Tab:
        return KeyCode::Tab;
    case XK_Shift_L:
        return KeyCode::LeftShift;
    case XK_Shift_R:
        return KeyCode::RightShift;
    case XK_Control_L:
        return KeyCode::LeftControl;
    case XK_Control_R:
        return KeyCode::RightControl;
    case XK_Meta_L:
    case XK_Alt_L:
        return KeyCode::LeftAlt;
    case XK_Mode_switch:      // Mapped to Alt_R on many keyboards
    case XK_ISO_Level3_Shift: // AltGr on at least some machines
    case XK_Meta_R:
    case XK_Alt_R:
        return KeyCode::RightAlt;
    case XK_Super_L:
        return KeyCode::LeftSuper;
    case XK_Super_R:
        return KeyCode::RightSuper;
    case XK_Menu:
        return KeyCode::Menu;
    case XK_Num_Lock:
        return KeyCode::NumLock;
    case XK_Caps_Lock:
        return KeyCode::CapsLock;
    case XK_Print:
        return KeyCode::PrintScreen;
    case XK_Scroll_Lock:
        return KeyCode::ScrollLock;
    case XK_Pause:
        return KeyCode::Pause;
    case XK_Delete:
        return KeyCode::Del;
    case XK_BackSpace:
        return KeyCode::Backspace;
    case XK_Return:
        return KeyCode::Enter;
    case XK_Home:
        return KeyCode::Home;
    case XK_End:
        return KeyCode::End;
    case XK_Page_Up:
        return KeyCode::PageUp;
    case XK_Page_Down:
        return KeyCode::PageDown;
    case XK_Insert:
        return KeyCode::Insert;
    case XK_Left:
        return KeyCode::Left;
    case XK_Right:
        return KeyCode::Right;
    case XK_Down:
        return KeyCode::Down;
    case XK_Up:
        return KeyCode::Up;
    case XK_F1:
        return KeyCode::F1;
    case XK_F2:
        return KeyCode::F2;
    case XK_F3:
        return KeyCode::F3;
    case XK_F4:
        return KeyCode::F4;
    case XK_F5:
        return KeyCode::F5;
    case XK_F6:
        return KeyCode::F6;
    case XK_F7:
        return KeyCode::F7;
    case XK_F8:
        return KeyCode::F8;
    case XK_F9:
        return KeyCode::F9;
    case XK_F10:
        return KeyCode::F10;
    case XK_F11:
        return KeyCode::F11;
    case XK_F12:
        return KeyCode::F12;
    case XK_F13:
        return KeyCode::F13;
    case XK_F14:
        return KeyCode::F14;
    case XK_F15:
        return KeyCode::F15;
    case XK_F16:
        return KeyCode::F16;
    case XK_F17:
        return KeyCode::F17;
    case XK_F18:
        return KeyCode::F18;
    case XK_F19:
        return KeyCode::F19;
    case XK_F20:
        return KeyCode::F20;
    case XK_F21:
        return KeyCode::F21;
    case XK_F22:
        return KeyCode::F22;
    case XK_F23:
        return KeyCode::F23;
    case XK_F24:
        return KeyCode::F24;
    case XK_F25:
        return KeyCode::F25;

    // Numeric keypad
    case XK_KP_Divide:
        return KeyCode::KPDivide;
    case XK_KP_Multiply:
        return KeyCode::KPMultiply;
    case XK_KP_Subtract:
        return KeyCode::KPSubtract;
    case XK_KP_Add:
        return KeyCode::KPAdd;

    // These should have been detected in secondary keysym test above!
    case XK_KP_Insert:
        return KeyCode::KP0;
    case XK_KP_End:
        return KeyCode::KP1;
    case XK_KP_Down:
        return KeyCode::KP2;
    case XK_KP_Page_Down:
        return KeyCode::KP3;
    case XK_KP_Left:
        return KeyCode::KP4;
    case XK_KP_Right:
        return KeyCode::KP6;
    case XK_KP_Home:
        return KeyCode::KP7;
    case XK_KP_Up:
        return KeyCode::KP8;
    case XK_KP_Page_Up:
        return KeyCode::KP9;
    case XK_KP_Delete:
        return KeyCode::KPDecimal;
    case XK_KP_Equal:
        return KeyCode::KPEqual;
    case XK_KP_Enter:
        return KeyCode::KPEnter;

    // Last resort: Check for printable keys (should not happen if the XKB
    // extension is available). This will give a layout dependent mapping
    // (which is wrong, and we may miss some keys, especially on non-US
    // keyboards), but it's better than nothing...
    case XK_a:
        return KeyCode::A;
    case XK_b:
        return KeyCode::B;
    case XK_c:
        return KeyCode::C;
    case XK_d:
        return KeyCode::D;
    case XK_e:
        return KeyCode::E;
    case XK_f:
        return KeyCode::F;
    case XK_g:
        return KeyCode::G;
    case XK_h:
        return KeyCode::H;
    case XK_i:
        return KeyCode::I;
    case XK_j:
        return KeyCode::J;
    case XK_k:
        return KeyCode::K;
    case XK_l:
        return KeyCode::L;
    case XK_m:
        return KeyCode::M;
    case XK_n:
        return KeyCode::N;
    case XK_o:
        return KeyCode::O;
    case XK_p:
        return KeyCode::P;
    case XK_q:
        return KeyCode::Q;
    case XK_r:
        return KeyCode::R;
    case XK_s:
        return KeyCode::S;
    case XK_t:
        return KeyCode::T;
    case XK_u:
        return KeyCode::U;
    case XK_v:
        return KeyCode::V;
    case XK_w:
        return KeyCode::W;
    case XK_x:
        return KeyCode::X;
    case XK_y:
        return KeyCode::Y;
    case XK_z:
        return KeyCode::Z;
    case XK_1:
        return KeyCode::Digit1;
    case XK_2:
        return KeyCode::Digit2;
    case XK_3:
        return KeyCode::Digit3;
    case XK_4:
        return KeyCode::Digit4;
    case XK_5:
        return KeyCode::Digit5;
    case XK_6:
        return KeyCode::Digit6;
    case XK_7:
        return KeyCode::Digit7;
    case XK_8:
        return KeyCode::Digit8;
    case XK_9:
        return KeyCode::Digit9;
    case XK_0:
        return KeyCode::Digit0;
    case XK_space:
        return KeyCode::Space;
    case XK_minus:
        return KeyCode::Minus;
    case XK_equal:
        return KeyCode::Equal;
    case XK_bracketleft:
        return KeyCode::LeftBracket;
    case XK_bracketright:
        return KeyCode::RightBracket;
    case XK_backslash:
        return KeyCode::Backslash;
    case XK_semicolon:
        return KeyCode::Semicolon;
    case XK_apostrophe:
        return KeyCode::Apostrophe;
    case XK_grave:
        return KeyCode::GraveAccent;
    case XK_comma:
        return KeyCode::Comma;
    case XK_period:
        return KeyCode::Period;
    case XK_slash:
        return KeyCode::Slash;
    case XK_less:
        return KeyCode::World1; // At least in some layouts...
    default:
        break;
    }

    // No matching translation was found
    return KeyCode::Unknown;
}

static void createKeyTables() {
    int scancodeMin, scancodeMax;

    memset(scanCodeToKeyCodeTable.data(), -1, sizeof(scanCodeToKeyCodeTable));
    memset(keyCodeToScanCodeTable.data(), -1, sizeof(keyCodeToScanCodeTable));

    if (xkb.available) {
        // Use XKB to determine physical key locations independently of the
        // current keyboard layout

        XkbDescPtr desc = XkbGetMap(display, 0, XkbUseCoreKbd);
        XkbGetNames(display, XkbKeyNamesMask | XkbKeyAliasesMask, desc);

        scancodeMin = desc->min_key_code;
        scancodeMax = desc->max_key_code;

        const struct {
            KeyCode key;
            const char* name;
        } keymap[] = { { KeyCode::GraveAccent, "TLDE" },
                       { KeyCode::Digit1, "AE01" },
                       { KeyCode::Digit2, "AE02" },
                       { KeyCode::Digit3, "AE03" },
                       { KeyCode::Digit4, "AE04" },
                       { KeyCode::Digit5, "AE05" },
                       { KeyCode::Digit6, "AE06" },
                       { KeyCode::Digit7, "AE07" },
                       { KeyCode::Digit8, "AE08" },
                       { KeyCode::Digit9, "AE09" },
                       { KeyCode::Digit0, "AE10" },
                       { KeyCode::Minus, "AE11" },
                       { KeyCode::Equal, "AE12" },
                       { KeyCode::Q, "AD01" },
                       { KeyCode::W, "AD02" },
                       { KeyCode::E, "AD03" },
                       { KeyCode::R, "AD04" },
                       { KeyCode::T, "AD05" },
                       { KeyCode::Y, "AD06" },
                       { KeyCode::U, "AD07" },
                       { KeyCode::I, "AD08" },
                       { KeyCode::O, "AD09" },
                       { KeyCode::P, "AD10" },
                       { KeyCode::LeftBracket, "AD11" },
                       { KeyCode::RightBracket, "AD12" },
                       { KeyCode::A, "AC01" },
                       { KeyCode::S, "AC02" },
                       { KeyCode::D, "AC03" },
                       { KeyCode::F, "AC04" },
                       { KeyCode::G, "AC05" },
                       { KeyCode::H, "AC06" },
                       { KeyCode::J, "AC07" },
                       { KeyCode::K, "AC08" },
                       { KeyCode::L, "AC09" },
                       { KeyCode::Semicolon, "AC10" },
                       { KeyCode::Apostrophe, "AC11" },
                       { KeyCode::Z, "AB01" },
                       { KeyCode::X, "AB02" },
                       { KeyCode::C, "AB03" },
                       { KeyCode::V, "AB04" },
                       { KeyCode::B, "AB05" },
                       { KeyCode::N, "AB06" },
                       { KeyCode::M, "AB07" },
                       { KeyCode::Comma, "AB08" },
                       { KeyCode::Period, "AB09" },
                       { KeyCode::Slash, "AB10" },
                       { KeyCode::Backslash, "BKSL" },
                       { KeyCode::World1, "LSGT" },
                       { KeyCode::Space, "SPCE" },
                       { KeyCode::Escape, "ESC" },
                       { KeyCode::Enter, "RTRN" },
                       { KeyCode::Tab, "TAB" },
                       { KeyCode::Backspace, "BKSP" },
                       { KeyCode::Insert, "INS" },
                       { KeyCode::Del, "DELE" },
                       { KeyCode::Right, "RGHT" },
                       { KeyCode::Left, "LEFT" },
                       { KeyCode::Down, "DOWN" },
                       { KeyCode::Up, "UP" },
                       { KeyCode::PageUp, "PGUP" },
                       { KeyCode::PageDown, "PGDN" },
                       { KeyCode::Home, "HOME" },
                       { KeyCode::End, "END" },
                       { KeyCode::CapsLock, "CAPS" },
                       { KeyCode::ScrollLock, "SCLK" },
                       { KeyCode::NumLock, "NMLK" },
                       { KeyCode::PrintScreen, "PRSC" },
                       { KeyCode::Pause, "PAUS" },
                       { KeyCode::F1, "FK01" },
                       { KeyCode::F2, "FK02" },
                       { KeyCode::F3, "FK03" },
                       { KeyCode::F4, "FK04" },
                       { KeyCode::F5, "FK05" },
                       { KeyCode::F6, "FK06" },
                       { KeyCode::F7, "FK07" },
                       { KeyCode::F8, "FK08" },
                       { KeyCode::F9, "FK09" },
                       { KeyCode::F10, "FK10" },
                       { KeyCode::F11, "FK11" },
                       { KeyCode::F12, "FK12" },
                       { KeyCode::F13, "FK13" },
                       { KeyCode::F14, "FK14" },
                       { KeyCode::F15, "FK15" },
                       { KeyCode::F16, "FK16" },
                       { KeyCode::F17, "FK17" },
                       { KeyCode::F18, "FK18" },
                       { KeyCode::F19, "FK19" },
                       { KeyCode::F20, "FK20" },
                       { KeyCode::F21, "FK21" },
                       { KeyCode::F22, "FK22" },
                       { KeyCode::F23, "FK23" },
                       { KeyCode::F24, "FK24" },
                       { KeyCode::F25, "FK25" },
                       { KeyCode::KP0, "KP0" },
                       { KeyCode::KP1, "KP1" },
                       { KeyCode::KP2, "KP2" },
                       { KeyCode::KP3, "KP3" },
                       { KeyCode::KP4, "KP4" },
                       { KeyCode::KP5, "KP5" },
                       { KeyCode::KP6, "KP6" },
                       { KeyCode::KP7, "KP7" },
                       { KeyCode::KP8, "KP8" },
                       { KeyCode::KP9, "KP9" },
                       { KeyCode::KPDecimal, "KPDL" },
                       { KeyCode::KPDivide, "KPDV" },
                       { KeyCode::KPMultiply, "KPMU" },
                       { KeyCode::KPSubtract, "KPSU" },
                       { KeyCode::KPAdd, "KPAD" },
                       { KeyCode::KPEnter, "KPEN" },
                       { KeyCode::KPEqual, "KPEQ" },
                       { KeyCode::LeftShift, "LFSH" },
                       { KeyCode::LeftControl, "LCTL" },
                       { KeyCode::LeftAlt, "LALT" },
                       { KeyCode::LeftSuper, "LWIN" },
                       { KeyCode::RightShift, "RTSH" },
                       { KeyCode::RightControl, "RCTL" },
                       { KeyCode::RightAlt, "RALT" },
                       { KeyCode::RightAlt, "LVL3" },
                       { KeyCode::RightAlt, "MDSW" },
                       { KeyCode::RightSuper, "RWIN" },
                       { KeyCode::Menu, "MENU" } };

        // Find the X11 key code -> key code mapping
        for (int scancode = scancodeMin; scancode <= scancodeMax; scancode++) {
            KeyCode key = KeyCode::Unknown;

            // Map the key name to a key code. Note: We use the US
            // keyboard layout. Because function keys aren't mapped correctly
            // when using traditional KeySym translations, they are mapped
            // here instead.
            for (int i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++) {
                if (strncmp(desc->names->keys[scancode].name, keymap[i].name, XkbKeyNameLength) == 0) {
                    key = keymap[i].key;
                    break;
                }
            }

            // Fall back to key aliases in case the key name did not match
            for (int i = 0; i < desc->names->num_key_aliases; i++) {
                if (key != KeyCode::Unknown)
                    break;

                if (strncmp(desc->names->key_aliases[i].real, desc->names->keys[scancode].name,
                            XkbKeyNameLength) != 0) {
                    continue;
                }

                for (int j = 0; j < sizeof(keymap) / sizeof(keymap[0]); j++) {
                    if (strncmp(desc->names->key_aliases[i].alias, keymap[j].name, XkbKeyNameLength) == 0) {
                        key = keymap[j].key;
                        break;
                    }
                }
            }

            scanCodeToKeyCodeTable[scancode] = key;
        }

        XkbFreeNames(desc, XkbKeyNamesMask, True);
        XkbFreeKeyboard(desc, 0, True);
    } else {
        XDisplayKeycodes(display, &scancodeMin, &scancodeMax);
    }

    int width;
    KeySym* keysyms = XGetKeyboardMapping(display, scancodeMin, scancodeMax - scancodeMin + 1, &width);

    for (int scancode = scancodeMin; scancode <= scancodeMax; scancode++) {
        // Translate the un-translated key codes using traditional X11 KeySym
        // lookups
        if (scanCodeToKeyCodeTable[scancode] < KeyCode(0)) {
            const size_t base                = (scancode - scancodeMin) * width;
            scanCodeToKeyCodeTable[scancode] = translateKeySyms(&keysyms[base], width);
        }

        // Store the reverse translation for faster key name lookup
        if (scanCodeToKeyCodeTable[scancode] > KeyCode(0))
            keyCodeToScanCodeTable[+scanCodeToKeyCodeTable[scancode]] = scancode;
    }

    XFree(keysyms);
}

} // namespace X11

/*static*/ void PlatformWindow::initialize() {
    X11::initialize();
}

/*static*/ void PlatformWindow::finalize() {
    X11::terminate();
}

void PlatformWindow::setWindowIcon() {
    //
}

void PlatformWindow::getHandle(OSWindowHandle& handle) {
    handle.display = X11::display;
    handle.window  = m_data->handle;
}

Bytes PlatformWindow::placement() const {
    return {};
}

void PlatformWindow::setPlacement(bytes_view data) {}

void PlatformWindow::setOwner(RC<Window> window) {}

bool PlatformWindow::createWindow() {
    Size size               = max(m_windowSize, Size{ 1, 1 });
    Point initialPos        = m_position;
    initialPos.x            = initialPos.x == dontCare ? 0 : initialPos.x;
    initialPos.y            = initialPos.y == dontCare ? 0 : initialPos.y;

    ::Visual* visual        = DefaultVisual(X11::display, X11::screen);
    int depth               = DefaultDepth(X11::display, X11::screen);

    // Create a colormap based on the visual used by the current context
    m_data->colormap        = XCreateColormap(X11::display, X11::root, visual, AllocNone);

    XSetWindowAttributes wa = { 0 };
    wa.colormap             = m_data->colormap;
    wa.event_mask           = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                    ButtonPressMask | ButtonReleaseMask | ExposureMask | FocusChangeMask |
                    VisibilityChangeMask | EnterWindowMask | LeaveWindowMask | PropertyChangeMask;

    X11::grabErrorHandlerX11();

    m_data->parent = X11::root;
    m_data->handle =
        XCreateWindow(X11::display, X11::root, initialPos.x, initialPos.y, size.width, size.height,
                      0,     // Border width
                      depth, // Color depth
                      InputOutput, visual, CWBorderPixel | CWColormap | CWEventMask, &wa);

    X11::releaseErrorHandlerX11();

    if (!m_data->handle) {
        LOG_ERROR(x11, "Failed to create window");
        return false;
    }

    XSaveContext(X11::display, m_data->handle, X11::context, (XPointer)this);

    if (m_windowStyle && WindowStyle::Undecorated)
        X11::setStyleDecorated(m_data.get(), false);

    if (X11::NET_WM_STATE) {
        Atom states[3];
        int count = 0;

        if (m_windowStyle && WindowStyle::TopMost) {
            if (X11::NET_WM_STATE_ABOVE)
                states[count++] = X11::NET_WM_STATE_ABOVE;
        }

        if (count) {
            XChangeProperty(X11::display, m_data->handle, X11::NET_WM_STATE, XA_ATOM, 32, PropModeReplace,
                            (unsigned char*)states, count);
        }
    }

    // Declare the WM protocols supported
    Atom protocols[] = { X11::WM_DELETE_WINDOW, X11::NET_WM_PING };
    XSetWMProtocols(X11::display, m_data->handle, protocols, std::size(protocols));

    // Declare our PID
    const long pid = getpid();

    XChangeProperty(X11::display, m_data->handle, X11::NET_WM_PID, XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char*)&pid, 1);

    if (X11::NET_WM_WINDOW_TYPE && X11::NET_WM_WINDOW_TYPE_NORMAL) {
        Atom type = X11::NET_WM_WINDOW_TYPE_NORMAL;
        XChangeProperty(X11::display, m_data->handle, X11::NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace,
                        (unsigned char*)&type, 1);
    }

    // Set ICCCM WM_HINTS property
    {
        XWMHints* hints = XAllocWMHints();
        BRISK_ASSERT(hints);
        SCOPE_EXIT {
            XFree(hints);
        };

        hints->flags         = StateHint;
        hints->initial_state = NormalState;

        XSetWMHints(X11::display, m_data->handle, hints);
    }

    // Set ICCCM WM_NORMAL_HINTS property
    {
        XSizeHints* hints = XAllocSizeHints();
        BRISK_ASSERT(hints);
        SCOPE_EXIT {
            XFree(hints);
        };

        if (!(m_windowStyle && WindowStyle::Resizable)) {
            hints->flags |= (PMinSize | PMaxSize);
            hints->min_width = hints->max_width = size.width;
            hints->min_height = hints->max_height = size.height;
        }

        // HACK: Explicitly setting PPosition to any value causes some WMs, notably
        //       Compiz and Metacity, to honor the position of unmapped windows
        if (initialPos.x != dontCare && initialPos.y != dontCare) {
            hints->flags |= PPosition;
            hints->x = 0;
            hints->y = 0;
        }

        hints->flags |= PWinGravity;
        hints->win_gravity = StaticGravity;

        XSetWMNormalHints(X11::display, m_data->handle, hints);
    }

    // Set ICCCM WM_CLASS property
    {
        XClassHint* hint = XAllocClassHint();
        BRISK_ASSERT(hint);
        SCOPE_EXIT {
            XFree(hint);
        };

        hint->res_name  = (char*)appMetadata.name.c_str();
        hint->res_class = (char*)"BRISK-APP";

        XSetClassHint(X11::display, m_data->handle, hint);
    }

    // Announce support for Xdnd (drag and drop)
    {
        const Atom version = XDND_VERSION;
        XChangeProperty(X11::display, m_data->handle, X11::XdndAware, XA_ATOM, 32, PropModeReplace,
                        (unsigned char*)&version, 1);
    }

    if (X11::im)
        X11::createInputContext(this);

    X11::setWindowTitle(m_data.get(), m_window->m_title);
    m_position        = X11::getWindowPos(m_data.get());
    m_windowSize      = X11::getWindowSize(m_data.get());
    m_framebufferSize = m_windowSize;

    XFlush(X11::display);
    return true;
}

PlatformWindow::~PlatformWindow() {
    std::erase(platformWindows, this);
    if (m_data->ic) {
        XDestroyIC(m_data->ic);
        m_data->ic = NULL;
    }

    if (m_data->handle) {
        XDeleteContext(X11::display, m_data->handle, X11::context);
        XUnmapWindow(X11::display, m_data->handle);
        XDestroyWindow(X11::display, m_data->handle);
        m_data->handle = (X11::Window)0;
    }

    if (m_data->colormap) {
        XFreeColormap(X11::display, m_data->colormap);
        m_data->colormap = (Colormap)0;
    }

    XFlush(X11::display);
}

PlatformWindow::PlatformWindow(Window* window, Size windowSize, Point position, WindowStyle style)
    : m_data(new PlatformWindowData{}), m_window(window), m_windowStyle(style), m_windowSize(windowSize),
      m_position(position) {
    mustBeMainThread();
    BRISK_ASSERT(m_window);

    bool created = createWindow();
    BRISK_SOFT_ASSERT(created);
    if (!created)
        return;

    setWindowIcon();
    updateSize();
    contentScaleChanged(m_scale, m_scale);

    platformWindows.push_back(this);
}

void PlatformWindow::setTitle(std::string_view title) {
    X11::setWindowTitle(m_data.get(), std::string(m_window->m_title));
}

void PlatformWindow::setSize(Size size) {
    X11::setWindowSize(this, size);
}

void PlatformWindow::setPosition(Point point) {
    X11::setWindowPos(this, point);
}

void PlatformWindow::setSizeLimits(Size minSize, Size maxSize) {
    m_minSize = minSize;
    m_maxSize = maxSize;
    Size size = X11::getWindowSize(m_data.get());
    X11::updateNormalHints(this, size);
    XFlush(X11::display);
}

void PlatformWindow::setStyle(WindowStyle windowStyle) {
    if ((windowStyle && WindowStyle::Disabled) && !(m_windowStyle && WindowStyle::Disabled)) {
        // Release all keyboard keys and mouse buttons if the window becomes disabled
        releaseButtonsAndKeys();
    }

    m_windowStyle = windowStyle;
    X11::setStyleTopMost(this, m_windowStyle && WindowStyle::TopMost);
    X11::setStyleDecorated(m_data.get(), !(m_windowStyle && WindowStyle::Undecorated));
    Size size = X11::getWindowSize(m_data.get());
    X11::updateNormalHints(this, size);
}

bool PlatformWindow::cursorInContentArea() const {
    return X11::windowHovered(this);
}

namespace Internal {

RC<SystemCursor> PlatformCursors::cursorFromImage(const RC<Image>& image, Point point, float scale) {
    // TODO
    return {};
}

RC<SystemCursor> PlatformCursors::getSystemCursor(Cursor shape) {
    // TODO
    return {};
}
} // namespace Internal

void PlatformWindow::setCursor(Cursor cursor) {
    // TODO
}

bool PlatformWindow::isVisible() const {
    return X11::windowVisible(this);
}

void PlatformWindow::iconify() {
    X11::iconifyWindow(this);
}

void PlatformWindow::maximize() {
    X11::maximizeWindow(this);
}

void PlatformWindow::restore() {
    X11::restoreWindow(this);
}

void PlatformWindow::focus() {
    X11::focusWindow(this);
}

bool PlatformWindow::isFocused() const {
    return X11::windowFocused(this);
}

bool PlatformWindow::isIconified() const {
    return X11::windowIconified(this);
}

bool PlatformWindow::isMaximized() const {
    return X11::windowMaximized(this);
}

void PlatformWindow::updateVisibility() {
    bool visible = m_window->m_visible;
    if (visible) {
        X11::showWindow(this);
        X11::focusWindow(this);
    } else {
        X11::hideWindow(this);
    }
}

/* static */ void PlatformWindow::pollEvents() {
    X11::pollEvents();
}

/* static */ void PlatformWindow::waitEvents() {
    X11::waitForAnyEvent(NULL);
    X11::pollEvents();
}

/* static */ void PlatformWindow::postEmptyEvent() {
    X11::writeEmptyEvent();
}

/* static */ DblClickParams PlatformWindow::dblClickParams() {
    return { 0.5, 2 };
}

} // namespace Brisk
