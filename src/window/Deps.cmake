get_property(
    _BRISK_WINDOW
    TARGET brisk-window
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_WINDOW}" STREQUAL "")
    set(_BRISK_WINDOW brisk-window)
endif ()

if (NOT WIN32 AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(${_BRISK_WINDOW} ${_DEP_PUBLIC} X11::X11 X11::Xrandr X11::Xinerama)
endif ()
