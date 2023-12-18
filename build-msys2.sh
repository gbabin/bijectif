#!/bin/bash
set -eo pipefail
IFS=$'\n\t'

if [ "$MSYSTEM" != 'UCRT64' ]
then
    echo 'Error: This script expects to be run in the MSYS2 UCRT64 environment.'
    exit 1
fi

pacman -S --needed --noconfirm \
       mingw-w64-ucrt-x86_64-{cmake,ninja,gcc} \
       mingw-w64-ucrt-x86_64-qt6-{base,imageformats,multimedia,tools,translations} \
       zip

rm -rf build/msys2
mkdir -p build/msys2/Bijectif

cmake -DCMAKE_BUILD_TYPE=Release . -G Ninja -B ./build/msys2
cmake --build ./build/msys2

cp build/msys2/*.exe build/msys2/Bijectif
cp build/msys2/*.qm build/msys2/Bijectif

cd build/msys2/Bijectif
windeployqt-qt6 ./*.exe
ldd ./*.exe imageformats/*.dll multimedia/*.dll sqldrivers/qsqlite.dll | grep -F ' => /ucrt64/' | awk '{print $3}' | sort --unique | xargs -I_ cp _ .
cd ..
zip -r9 Bijectif.zip Bijectif
realpath Bijectif.zip
