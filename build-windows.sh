#!/bin/bash
set -e

echo "🪟 Building for Windows deployment (using system packages)..."
mkdir -p build-windows
cd build-windows

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw.cmake \
    -DUSE_SYSTEM_PACKAGES=ON

make -j$(nproc)
echo "✅ Windows build complete!"
