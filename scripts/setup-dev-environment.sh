#!/bin/bash
# Setup script for Codice-Cam development environment on Linux
# This script sets up cross-platform development for Linux -> Windows deployment

set -e  # Exit on any error

echo "🚀 Setting up Codice-Cam development environment..."

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
    libsdl2-mixer-dev

# Create vcpkg directory
VCPKG_DIR="$(pwd)/vcpkg"
if [ ! -d "$VCPKG_DIR" ]; then
    echo "📥 Cloning vcpkg..."
    git clone https://github.com/Microsoft/vcpkg.git "$VCPKG_DIR"
    cd "$VCPKG_DIR"
    ./bootstrap-vcpkg.sh
    cd ..
else
    echo "✅ vcpkg already exists, updating..."
    cd "$VCPKG_DIR"
    git pull
    ./bootstrap-vcpkg.sh
    cd ..
fi

# Install Linux packages
echo "🐧 Installing Linux packages..."
"$VCPKG_DIR/vcpkg" install \
    opencv4:x64-linux \
    sdl2:x64-linux \
    sdl2-image:x64-linux

# Install Windows packages for cross-compilation
echo "🪟 Installing Windows packages for cross-compilation..."
"$VCPKG_DIR/vcpkg" install \
    opencv4:x64-mingw-dynamic \
    sdl2:x64-mingw-dynamic \
    sdl2-image:x64-mingw-dynamic

# Create build directories
echo "📁 Creating build directories..."
mkdir -p build-linux
mkdir -p build-windows

# Create build scripts
echo "📝 Creating build scripts..."

# Linux build script
cat > build-linux.sh << 'EOF'
#!/bin/bash
set -e

echo "🐧 Building for Linux development..."
mkdir -p build-linux
cd build-linux

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DVCPKG_TARGET_TRIPLET=x64-linux

make -j$(nproc)
echo "✅ Linux build complete!"
EOF

# Windows build script
cat > build-windows.sh << 'EOF'
#!/bin/bash
set -e

echo "🪟 Building for Windows deployment..."
mkdir -p build-windows
cd build-windows

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

make -j$(nproc)
echo "✅ Windows build complete!"
EOF

# Make scripts executable
chmod +x build-linux.sh build-windows.sh

# Create development configuration
echo "⚙️ Creating development configuration..."
cat > .vscode/settings.json << 'EOF'
{
    "cmake.configureArgs": [
        "-DCMAKE_TOOLCHAIN_FILE=${workspaceFolder}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    ],
    "cmake.buildDirectory": "${workspaceFolder}/build-linux",
    "cmake.generator": "Unix Makefiles",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.compilerPath": "/usr/bin/gcc"
}
EOF

# Create CMake toolchain file for cross-compilation
echo "🔧 Creating CMake toolchain file..."
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
echo "🎉 Development environment setup complete!"
echo ""
echo "📋 Next steps:"
echo "1. Run './build-linux.sh' to build for Linux development"
echo "2. Run './build-windows.sh' to build for Windows deployment"
echo "3. Start coding in src/ directory"
echo ""
echo "🔧 Available commands:"
echo "  ./build-linux.sh    - Build for Linux development"
echo "  ./build-windows.sh  - Build for Windows deployment"
echo "  ./vcpkg/vcpkg list  - List installed packages"
echo ""
echo "📚 Documentation:"
echo "  - PRD.md: Product requirements"
echo "  - ARCHITECTURE.md: System design"
echo "  - TECH_STACK.md: Technology choices"
echo "  - TASKS.md: Development plan"
echo ""
echo "Happy coding! 🚀"
