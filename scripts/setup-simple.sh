#!/bin/bash
# Simple setup script for Codice-Cam development environment
# Uses system packages instead of vcpkg for faster setup

set -e  # Exit on any error

echo "🚀 Setting up Codice-Cam development environment (Simple mode)..."

# Check if running on Linux
if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo "❌ This script is designed for Linux development environments"
    exit 1
fi

# Update package list
echo "📦 Updating package list..."
sudo apt update

# Install essential development tools
echo "🔧 Installing development tools..."
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    curl \
    unzip

# Install MinGW-w64 for cross-compilation
echo "🪟 Installing MinGW-w64 for Windows cross-compilation..."
sudo apt install -y \
    mingw-w64 \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64

# Install system dependencies for OpenCV and SDL2
echo "📚 Installing system dependencies..."
sudo apt install -y \
    libopencv-dev \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev \
    libsdl2-mixer-dev \
    libgtk-3-dev \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libjpeg-dev \
    libpng-dev \
    libtiff-dev \
    libwebp-dev

# Create build directories
echo "📁 Creating build directories..."
mkdir -p build-linux
mkdir -p build-windows

# Create build scripts
echo "📝 Creating build scripts..."

# Linux build script (using system packages)
cat > build-linux.sh << 'EOF'
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
EOF

# Windows build script (using system packages)
cat > build-windows.sh << 'EOF'
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
EOF

# Make scripts executable
chmod +x build-linux.sh build-windows.sh

# Create development configuration
echo "⚙️ Creating development configuration..."
mkdir -p .vscode
cat > .vscode/settings.json << 'EOF'
{
    "cmake.configureArgs": [
        "-DUSE_SYSTEM_PACKAGES=ON"
    ],
    "cmake.buildDirectory": "${workspaceFolder}/build-linux",
    "cmake.generator": "Unix Makefiles",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.compilerPath": "/usr/bin/gcc"
}
EOF

# Create CMake toolchain file for cross-compilation
echo "🔧 Creating CMake toolchain file..."
mkdir -p cmake
cat > cmake/toolchain-mingw.cmake << 'EOF'
# MinGW-w64 cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

# Where is the target environment located
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Adjust the default behavior of the FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set the resource compiler
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Set the archiver
set(CMAKE_AR x86_64-w64-mingw32-ar)
set(CMAKE_RANLIB x86_64-w64-mingw32-ranlib)
EOF

echo ""
echo "🎉 Simple development environment setup complete!"
echo ""
echo "📋 Next steps:"
echo "1. Run './build-linux.sh' to build for Linux development"
echo "2. Run './build-windows.sh' to build for Windows deployment"
echo "3. Start coding in src/ directory"
echo ""
echo "🔧 Available commands:"
echo "  ./build-linux.sh    - Build for Linux development"
echo "  ./build-windows.sh  - Build for Windows deployment"
echo ""
echo "💡 Note: This setup uses system packages for faster installation."
echo "   For production builds, consider using vcpkg for better dependency management."
echo ""
echo "📚 Documentation:"
echo "  - PRD.md: Product requirements"
echo "  - ARCHITECTURE.md: System design"
echo "  - TECH_STACK.md: Technology choices"
echo "  - TASKS.md: Development plan"
echo ""
echo "Happy coding! 🚀"
