#!/bin/sh
shopt -s globstar

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd "$SCRIPT_DIR"

find . \( -name '*.hpp' -or -name '*.cpp' -or -name '*.mm' \) -not -path "./build/*" -not -path "./vcpkg_installed/*" -not -path "./vcpkg_exported/*" -not -path "./src/graphics/vector/*" -not -path "./deps/*" -not -path "./src/gui/yoga/*" -not -path "./src/core/cityhash/*" -exec clang-format -i {} \;
