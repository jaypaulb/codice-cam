# Product Requirements Document (PRD)
## Codice-Cam: Webcam-based Codice Marker Detection for TUIO

### 1. Project Overview

**Project Name**: Codice-Cam  
**Version**: 1.0  
**Date**: January 2025  
**Target Platform**: Windows (Primary)  

### 2. Problem Statement

MultiTaction (MT) displays use proprietary Codice markers for object tracking, but the hardware and protocol are proprietary. Developers need a way to test and develop applications using Codice markers without access to the actual MT hardware.

### 3. Solution Overview

Codice-Cam is a computer vision application that:
- Captures video from a standard webcam
- Detects and decodes Codice markers in real-time
- Converts marker data to TUIO protocol format
- Streams TUIO data to MT Showcase software for testing

### 4. Functional Requirements

#### 4.1 Core Functionality
- **FR-001**: Capture video from webcam at 30+ FPS
- **FR-002**: Detect Codice markers (4x4 format) in video frames
- **FR-003**: Decode marker IDs from detected markers
- **FR-004**: Calculate marker position (x, y coordinates)
- **FR-005**: Calculate marker rotation angle
- **FR-006**: Support multiple simultaneous markers (up to 10)
- **FR-007**: Stream marker data via TUIO 1.1 protocol
- **FR-008**: Provide real-time visual feedback of detected markers

#### 4.2 Marker Detection Requirements
- **FR-009**: Support 4x4 Codice markers (4096 unique IDs)
- **FR-010**: Handle markers from 40mm to 100mm physical size
- **FR-011**: Detect markers at various angles (0-360°)
- **FR-012**: Maintain detection accuracy >95% under good lighting
- **FR-013**: Zero false positives (no spurious marker detection)
- **FR-014**: Sub-1 second detection latency acceptable

#### 4.3 TUIO Integration Requirements
- **FR-015**: Emit TUIO Object messages for detected markers
- **FR-016**: Use marker ID as TUIO symbol_id
- **FR-017**: Stream to default TUIO port (3333) or configurable port
- **FR-018**: Support UDP and TCP TUIO transport
- **FR-019**: Maintain TUIO session IDs for marker tracking

### 5. Non-Functional Requirements

#### 5.1 Performance
- **NFR-001**: Process video at 30+ FPS
- **NFR-002**: Detection latency <1 second
- **NFR-003**: CPU usage <50% on modern hardware
- **NFR-004**: Memory usage <500MB

#### 5.2 Reliability
- **NFR-005**: Handle webcam disconnection gracefully
- **NFR-006**: Recover from detection errors without crashing
- **NFR-007**: Maintain stable TUIO stream during operation

#### 5.3 Usability
- **NFR-008**: Simple GUI for camera selection and settings
- **NFR-009**: Visual overlay showing detected markers
- **NFR-010**: Configuration persistence between sessions
- **NFR-011**: Clear error messages and status indicators

### 6. Technical Constraints

#### 6.1 Hardware Requirements
- **TC-001**: Standard USB webcam (720p minimum)
- **TC-002**: Windows 10/11 compatible
- **TC-003**: 4GB RAM minimum, 8GB recommended
- **TC-004**: Multi-core CPU recommended

#### 6.2 Software Dependencies
- **TC-005**: OpenCV 4.x for computer vision
- **TC-006**: TUIO11_CPP library for protocol handling
- **TC-007**: SDL2 for GUI and windowing
- **TC-008**: Visual Studio 2019+ or compatible compiler

### 7. Success Criteria

#### 7.1 Primary Success Metrics
- Successfully detect and decode Codice markers from webcam feed
- Stream TUIO data that MT Showcase software can consume
- Maintain stable operation for extended testing sessions
- Achieve >95% detection accuracy under optimal conditions

#### 7.2 Secondary Success Metrics
- Support for multiple simultaneous markers
- Configurable detection parameters
- Real-time visual feedback
- Easy setup and configuration

### 8. Out of Scope

#### 8.1 Explicitly Excluded
- Support for 3x3 or 5x5 Codice markers (4x4 only)
- Marker generation/creation tools
- Integration with non-MT software
- Mobile or cross-platform support
- Advanced marker tracking features (velocity, acceleration)

#### 8.2 Future Considerations
- Support for additional marker formats
- Cross-platform compatibility
- Advanced filtering and smoothing
- Marker calibration tools

### 9. Dependencies and Assumptions

#### 9.1 External Dependencies
- MT Showcase software for testing TUIO integration
- Standard webcam drivers and hardware
- Windows development environment

#### 9.2 Assumptions
- Users have access to physical Codice markers for testing
- Adequate lighting conditions for marker detection
- Webcam positioned to capture markers clearly
- Basic understanding of TUIO protocol concepts

### 10. Risk Assessment

#### 10.1 Technical Risks
- **High**: Computer vision accuracy under varying lighting conditions
- **Medium**: Performance optimization for real-time processing
- **Low**: TUIO protocol integration complexity

#### 10.2 Mitigation Strategies
- Implement robust lighting compensation algorithms
- Use efficient OpenCV operations and optimization
- Leverage proven TUIO11_CPP reference implementation

---

**Document Status**: Draft  
**Next Review**: After architecture design completion  
**Approval Required**: Technical Lead, Product Owner
