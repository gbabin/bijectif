#!/bin/sh
set -euo pipefail
IFS=$'\n\t'

rm -rf build/mingw
mkdir -p build/mingw

podman run --userns=keep-id -it --rm \
       -v "${PWD}:/home/user/project:ro" \
       -v "${PWD}/build/mingw:/home/user/project/build/mingw:rw" \
       docker.io/stateoftheartio/qt6:6.5-mingw-aqt \
       sh -c 'cd /home/user/project;
              qt-cmake -DCMAKE_BUILD_TYPE=Release . -G Ninja -B ./build/mingw;
              cmake --build ./build/mingw;
              windeployqt -dir build/mingw/deploy --plugindir build/mingw/deploy/plugins build/mingw/*.exe --no-opengl-sw --no-system-d3d-compiler;
              cp build/mingw/*.exe build/mingw/deploy
              cp build/mingw/*.qm build/mingw/deploy'
