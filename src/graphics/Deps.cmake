# >PNG
find_package(PNG REQUIRED)
target_link_libraries(brisk-graphics ${_DEP_PRIVATE} PNG::PNG)
# /PNG

# >WebP
find_package(WebP CONFIG REQUIRED)
target_link_libraries(brisk-graphics ${_DEP_PRIVATE} WebP::webp WebP::webpdecoder)
# /WebP

# >libjpeg-turbo
find_package(libjpeg-turbo CONFIG REQUIRED)
target_link_libraries(
    brisk-graphics ${_DEP_PRIVATE}
    $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>)
# /libjpeg-turbo

# >lunasvg
find_package(unofficial-lunasvg CONFIG REQUIRED)
target_link_libraries(brisk-graphics ${_DEP_PRIVATE} unofficial::lunasvg::lunasvg)
# /lunasvg

# >harfbuzz
find_package(harfbuzz CONFIG REQUIRED)
target_link_libraries(brisk-graphics ${_DEP_PRIVATE} harfbuzz)
# /harfbuzz

# >freetype
find_package(freetype CONFIG REQUIRED)
target_link_libraries(brisk-graphics ${_DEP_PRIVATE} freetype)
# /freetype

# >icu
find_package(ICU COMPONENTS uc)
if (TARGET ICU::uc)
    target_link_libraries(brisk-graphics ${_DEP_PRIVATE} ICU::uc)
endif ()
# /icu

if (BRISK_WEBGPU)
    find_package(Dawn CONFIG)
    target_link_libraries(brisk-renderer-webgpu ${_DEP_PRIVATE} Dawn)
endif ()
