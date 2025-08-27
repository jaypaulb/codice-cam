#!/bin/bash
set -e

echo "🐧 Building for Linux development (using system packages)..."
mkdir -p build-linux
cd build-linux

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUSE_SYSTEM_PACKAGES=ON

make -j$(nproc)
echo "✅ Linux build complete!"
