if (NOT WIN32 AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(brisk-window ${_DEP_PUBLIC} X11::X11 X11::Xrandr X11::Xinerama)
endif ()
