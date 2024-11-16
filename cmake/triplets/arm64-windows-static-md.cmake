set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_C_FLAGS "-Zc:inline")

if (DEFINED ENV{LLVM_DIR})
    set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} -gline-tables-only")
endif ()

set(VCPKG_CXX_FLAGS ${VCPKG_C_FLAGS})

set(VCPKG_ENV_PASSTHROUGH_UNTRACKED "VCPKG_ROOT;LLVM_DIR")

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../toolchains/windows.cmake)

set(VCPKG_LOAD_VCVARS_ENV ON)
