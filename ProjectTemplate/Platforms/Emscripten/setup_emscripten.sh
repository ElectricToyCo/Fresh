#!/bin/sh

# Setup Emscripten build directory using CMake.
# Note that Emscripten builds don't support anything like -all_load,
# so you must insert code somewhere (main.cpp?) to directly reference
# any missing Fresh classes. Check the browser JavaScript logs
# for missing class complaints.

mkdir -p build
cp index.html build/index.html
cd build
cmake ../../.. -DCMAKE_MODULE_PATH="$EMSDK/fastcomp/emscripten/cmake/Modules" -DCMAKE_TOOLCHAIN_FILE="$EMSDK/fastcomp/emscripten/cmake/Modules/Platform/Emscripten.cmake" -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN=1 -G "Unix Makefiles"
make -j6 install

# To serve files locally, now run: `python3 -m http.server 8080`
# Then in a browser, visit: http://localhost:8080
