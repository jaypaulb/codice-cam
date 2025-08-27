# Technology Stack
## Codice-Cam: Webcam-based Codice Marker Detection for TUIO

### 1. Core Technology Decisions

#### 1.1 Programming Language: C++
**Rationale**:
- Performance-critical computer vision processing
- Excellent OpenCV integration and performance
- Direct access to TUIO11_CPP reference implementation
- Cross-platform potential for future expansion
- Lower-level control for optimization

**Alternatives Considered**:
- Python: Easier development but slower performance
- Java: Good TUIO support but limited OpenCV performance
- C#: Windows-specific, limited cross-platform potential

#### 1.2 Computer Vision Library: OpenCV 4.x
**Rationale**:
- Industry standard for computer vision
- Excellent C++ bindings and performance
- Robust camera capture and image processing
- Extensive documentation and community support
- Built-in marker detection capabilities (Aruco, etc.)

**Key Features Used**:
- VideoCapture for webcam input
- Image preprocessing (thresholding, morphology)
- Contour detection and analysis
- Perspective transformation
- Template matching

#### 1.3 TUIO Protocol: TUIO11_CPP
**Rationale**:
- Official TUIO 1.1 reference implementation
- Proven stability and compatibility
- Complete server and client implementations
- Multiple transport protocols (UDP, TCP, WebSocket)
- Well-documented API

**Source**: [https://github.com/mkalten/TUIO11_CPP](https://github.com/mkalten/TUIO11_CPP)

#### 1.4 Graphics and Windowing: SDL2
**Rationale**:
- Cross-platform graphics library
- Used by TUIO11_CPP examples
- Lightweight and efficient
- Good OpenGL integration
- Simple window management

### 2. Build System and Development Tools

#### 2.1 Build System: CMake
**Rationale**:
- Cross-platform build configuration
- Excellent dependency management
- Industry standard for C++ projects
- Good IDE integration
- Easy to extend and maintain

#### 2.2 Development Environment: Cross-Platform
**Development Platform**: Linux (Ubuntu/Debian)
**Target Platform**: Windows
**Rationale**:
- Linux provides superior development tools and faster compilation
- Cross-compilation to Windows using MinGW-w64 or Clang
- Cost-effective development environment
- Better CI/CD integration
- Docker support for consistent builds

**Development Tools**:
- **Linux**: GCC/Clang, CMake, vcpkg, Visual Studio Code
- **Windows**: MinGW-w64 cross-compiler, Windows SDK for deployment

#### 2.3 Package Management: Cross-Platform
**Linux Development**:
- **vcpkg**: Microsoft's C++ package manager (works on Linux)
- **apt/apt-get**: System package manager for dependencies
- **Conan**: Alternative C++ package manager

**Windows Deployment**:
- **vcpkg**: For Windows-specific builds
- **Windows SDK**: For final deployment packaging

**Rationale**:
- vcpkg works on both Linux and Windows
- Consistent dependency management across platforms
- Easy OpenCV and SDL2 installation
- CMake integration
- Handles complex cross-platform dependencies

### 3. Dependencies

#### 3.1 Core Dependencies
```
OpenCV 4.x
├── opencv_core
├── opencv_imgproc
├── opencv_imgcodecs
├── opencv_videoio
└── opencv_highgui

SDL2
├── SDL2
├── SDL2main
└── SDL2_image (optional)

TUIO11_CPP
├── TUIO library
└── oscpack (OpenSound Control)
```

#### 3.2 Development Dependencies
**Linux Development**:
```
CMake 3.16+
GCC 9+ or Clang 10+
Git (version control)
vcpkg (package manager)
MinGW-w64 (cross-compilation)
```

**Windows Deployment**:
```
Windows SDK
Visual Studio Build Tools (optional)
vcpkg (Windows packages)
```

### 4. Architecture Components

#### 4.1 Core Modules
```
CodiceCam/
├── src/
│   ├── Camera/           # Webcam capture and management
│   ├── Detection/        # Marker detection algorithms
│   ├── Decoding/         # Codice marker ID decoding
│   ├── TUIO/            # TUIO protocol integration
│   ├── GUI/             # User interface
│   └── Utils/           # Common utilities
├── include/             # Header files
├── tests/               # Unit and integration tests
├── docs/                # Documentation
└── resources/           # Assets and configuration
```

#### 4.2 Key Classes
```cpp
// Core detection pipeline
class CameraManager
class MarkerDetector
class CodiceDecoder
class TUIOBridge

// TUIO integration
class CodiceTUIOObject : public TuioObject
class TUIOStreamer

// GUI components
class MainWindow
class DetectionOverlay
class SettingsDialog
```

### 5. Performance Considerations

#### 5.1 Optimization Strategies
- **Multi-threading**: Separate camera capture and detection threads
- **Frame skipping**: Process every Nth frame for performance
- **ROI processing**: Focus detection on regions of interest
- **Memory pooling**: Reuse image buffers to reduce allocations
- **SIMD optimization**: Use OpenCV's optimized operations

#### 5.2 Resource Management
- **Camera buffers**: Limit frame buffer size
- **Image processing**: Use efficient data types (CV_8UC1)
- **Memory cleanup**: Proper RAII and smart pointers
- **Thread safety**: Mutex protection for shared data

### 6. Cross-Platform Build Configuration

#### 6.1 Linux Development Setup
```bash
# Install development tools
sudo apt update
sudo apt install build-essential cmake git

# Install MinGW-w64 for cross-compilation
sudo apt install mingw-w64

# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Install cross-platform packages
./vcpkg install opencv4:x64-linux
./vcpkg install sdl2:x64-linux
./vcpkg install opencv4:x64-mingw-dynamic
./vcpkg install sdl2:x64-mingw-dynamic
```

#### 6.2 Cross-Compilation Configuration
```cmake
# CMakeLists.txt cross-compilation setup
if(WIN32)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
```

#### 6.3 Build Scripts
```bash
#!/bin/bash
# build-linux.sh - Development build
mkdir -p build-linux
cd build-linux
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)

#!/bin/bash
# build-windows.sh - Cross-compilation build
mkdir -p build-windows
cd build-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
         -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic
make -j$(nproc)
```

### 7. Configuration and Deployment

#### 7.1 Configuration Files
```json
{
  "camera": {
    "device_id": 0,
    "resolution": "1280x720",
    "fps": 30
  },
  "detection": {
    "min_marker_size": 40,
    "max_marker_size": 100,
    "detection_threshold": 0.7
  },
  "tuio": {
    "host": "localhost",
    "port": 3333,
    "protocol": "UDP"
  }
}
```

#### 6.2 Build Configuration
```cmake
# CMakeLists.txt structure
cmake_minimum_required(VERSION 3.16)
project(CodiceCam)

# Find packages
find_package(OpenCV REQUIRED)
find_package(SDL2 REQUIRED)

# Include TUIO11_CPP
add_subdirectory(third_party/TUIO11_CPP)

# Build executable
add_executable(CodiceCam src/main.cpp ...)
target_link_libraries(CodiceCam OpenCV::OpenCV SDL2::SDL2 TUIO)
```

### 7. Testing Strategy

#### 7.1 Unit Testing
- **Framework**: Google Test (gtest)
- **Coverage**: Core detection algorithms, TUIO integration
- **Mock objects**: Camera input, TUIO output

#### 7.2 Integration Testing
- **Hardware testing**: Various webcam models
- **Marker testing**: Different sizes, angles, lighting conditions
- **TUIO testing**: MT Showcase software integration

#### 7.3 Performance Testing
- **Benchmarks**: Detection speed, memory usage
- **Stress testing**: Extended operation, multiple markers
- **Compatibility testing**: Different Windows versions

### 8. Future Technology Considerations

#### 8.1 Potential Upgrades
- **OpenCV 5.x**: When available, for improved performance
- **CUDA support**: GPU acceleration for detection
- **Machine learning**: Neural network-based detection
- **Cross-platform**: Linux/macOS support

#### 8.2 Alternative Approaches
- **Web-based**: WebRTC + JavaScript for browser deployment
- **Mobile**: iOS/Android native apps
- **Cloud processing**: Server-side detection with webcam streaming

---

**Document Status**: Draft
**Last Updated**: January 2025
**Review Cycle**: Quarterly or major technology changes
