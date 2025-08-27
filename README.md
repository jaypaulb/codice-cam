# Codice-Cam

A computer vision application that detects Codice markers via webcam and streams TUIO data to MultiTaction Showcase software.

## Overview

Codice-Cam bridges the gap between physical Codice markers and digital applications by:
- Capturing video from a standard webcam
- Detecting and decoding 4x4 Codice markers in real-time
- Converting marker data to TUIO 1.1 protocol format
- Streaming TUIO data to MT Showcase software for testing

## Features

- **Real-time Detection**: Processes video at 30+ FPS
- **Multiple Markers**: Supports up to 10 simultaneous markers
- **High Accuracy**: >95% detection accuracy under optimal conditions
- **TUIO Integration**: Compatible with TUIO 1.1 protocol
- **Configurable**: Adjustable detection parameters and camera settings
- **Visual Feedback**: Real-time overlay showing detected markers

## Requirements

### Hardware
- Windows 10/11 compatible system
- USB webcam (720p minimum resolution)
- 4GB RAM minimum, 8GB recommended
- Multi-core CPU recommended

### Software
- Visual Studio 2019+ or compatible C++ compiler
- CMake 3.16+
- vcpkg package manager

## Quick Start

### Prerequisites
1. Install Visual Studio 2019+ with C++ development tools
2. Install vcpkg package manager
3. Install required dependencies:
   ```bash
   vcpkg install opencv4:x64-windows
   vcpkg install sdl2:x64-windows
   vcpkg integrate install
   ```

### Building
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd codice-cam
   ```

2. Initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

3. Build with CMake:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

### Running
1. Connect a webcam to your system
2. Run the application:
   ```bash
   ./CodiceCam.exe
   ```
3. Configure camera and detection settings
4. Start detection and connect to MT Showcase software

## Configuration

The application uses a JSON configuration file for settings:

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

## Codice Markers

Codice markers are 4x4 grid markers with:
- **Format**: 4x4 binary grid (4096 unique IDs)
- **Size**: 40mm minimum, 50mm recommended
- **Material**: High-quality polyester recommended
- **Usage**: Place against display glass, no wrinkles

## TUIO Integration

The application streams TUIO 1.1 protocol data:
- **Default Port**: 3333 (UDP)
- **Protocol**: TUIO Object messages
- **Mapping**: Codice marker ID → TUIO symbol_id
- **Target**: MT Showcase software

## Development

### Project Structure
```
CodiceCam/
├── src/                 # Source code
├── include/             # Header files
├── tests/               # Unit and integration tests
├── docs/                # Documentation
├── resources/           # Assets and configuration
└── third_party/         # External dependencies
```

### Key Components
- **CameraManager**: Webcam capture and frame management
- **MarkerDetector**: Computer vision processing and detection
- **CodiceDecoder**: Marker ID decoding from binary grids
- **TUIOBridge**: TUIO protocol integration and streaming

### Testing
```bash
# Run unit tests
cd build
ctest --config Release

# Run specific test
./tests/CodiceCamTests.exe
```

## Documentation

- [Product Requirements Document](PRD.md)
- [System Architecture](ARCHITECTURE.md)
- [Technology Stack](TECH_STACK.md)
- [Development Tasks](TASKS.md)
- [Codice Markers Guide](Docs/Codice%20Markers.md)

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## License

This project is licensed under the LGPL-3.0 License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [TUIO11_CPP](https://github.com/mkalten/TUIO11_CPP) - TUIO 1.1 reference implementation
- [OpenCV](https://opencv.org/) - Computer vision library
- [SDL2](https://www.libsdl.org/) - Graphics and windowing library
- MultiTaction - Codice marker format and specifications

## Support

For issues and questions:
1. Check the documentation
2. Search existing issues
3. Create a new issue with detailed information

---

**Version**: 1.0.0  
**Last Updated**: January 2025
