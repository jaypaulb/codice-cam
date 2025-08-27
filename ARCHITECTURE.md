# System Architecture
## Codice-Cam: Webcam-based Codice Marker Detection for TUIO

### 1. High-Level Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Webcam        │    │   Codice-Cam    │    │   MT Showcase   │
│   Hardware      │───▶│   Application   │───▶│   Software      │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                              │
                              ▼
                       ┌─────────────────┐
                       │   TUIO Stream   │
                       │   (UDP/TCP)     │
                       └─────────────────┘
```

### 2. Component Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Codice-Cam Application                    │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │    GUI      │  │   Camera    │  │ Detection   │  │  TUIO   │ │
│  │  Manager    │  │  Manager    │  │  Pipeline   │  │ Bridge  │ │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────┘ │
│         │                │                │              │      │
│         ▼                ▼                ▼              ▼      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │   Settings  │  │   Capture   │  │   Marker    │  │ TUIO    │ │
│  │   Dialog    │  │   Thread    │  │  Detector   │  │ Server  │ │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────┘ │
│                           │                │              │      │
│                           ▼                ▼              ▼      │
│                    ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│                    │   Frame     │  │   Codice    │  │  OSC    │ │
│                    │   Buffer    │  │  Decoder    │  │ Sender  │ │
│                    └─────────────┘  └─────────────┘  └─────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### 3. Data Flow Architecture

```
Webcam → Frame Capture → Preprocessing → Detection → Decoding → TUIO → Network
   │           │              │            │          │         │        │
   ▼           ▼              ▼            ▼          ▼         ▼        ▼
Raw Video → Buffered → Enhanced → Contours → Marker → TUIO → UDP/TCP → MT
 Frames     Frames    Images     Found     IDs      Objects  Stream   Software
```

### 4. Core Components

#### 4.1 Camera Manager
**Responsibility**: Webcam capture and frame management
```cpp
class CameraManager {
public:
    bool initialize(int deviceId);
    bool startCapture();
    void stopCapture();
    cv::Mat getLatestFrame();
    bool isCapturing() const;
    
private:
    cv::VideoCapture camera;
    std::thread captureThread;
    std::mutex frameMutex;
    cv::Mat currentFrame;
    bool running;
};
```

**Key Features**:
- Thread-safe frame capture
- Configurable resolution and FPS
- Error handling and recovery
- Frame buffering for performance

#### 4.2 Marker Detection Pipeline
**Responsibility**: Computer vision processing and marker detection
```cpp
class MarkerDetector {
public:
    struct DetectionResult {
        std::vector<cv::Point2f> corners;
        int markerId;
        float confidence;
        cv::Mat perspective;
    };
    
    std::vector<DetectionResult> detectMarkers(const cv::Mat& frame);
    void setDetectionParameters(const DetectionConfig& config);
    
private:
    cv::Mat preprocessFrame(const cv::Mat& frame);
    std::vector<cv::Point2f> findMarkerContours(const cv::Mat& binary);
    bool validateMarker(const std::vector<cv::Point2f>& corners);
    cv::Mat extractMarkerRegion(const cv::Mat& frame, 
                               const std::vector<cv::Point2f>& corners);
};
```

**Detection Pipeline**:
1. **Preprocessing**: Grayscale conversion, noise reduction
2. **Thresholding**: Adaptive threshold for marker detection
3. **Contour Detection**: Find potential marker boundaries
4. **Validation**: Verify 4x4 grid structure
5. **Perspective Correction**: Normalize marker orientation
6. **ID Decoding**: Extract binary data from grid

#### 4.3 Codice Decoder
**Responsibility**: Decode marker IDs from detected grids
```cpp
class CodiceDecoder {
public:
    struct DecodedMarker {
        int id;
        float confidence;
        cv::Point2f center;
        float angle;
    };
    
    DecodedMarker decodeMarker(const cv::Mat& markerImage);
    bool validateMarkerFormat(const cv::Mat& binaryGrid);
    
private:
    std::vector<std::vector<bool>> extractGrid(const cv::Mat& image);
    int binaryToDecimal(const std::vector<bool>& bits);
    bool checkOrientation(const std::vector<std::vector<bool>>& grid);
};
```

**Decoding Process**:
1. **Grid Extraction**: Convert image to 4x4 binary grid
2. **Orientation Detection**: Use corner markers (tl=1, tr=0, bl=0, br=0)
3. **Data Extraction**: Read binary sequence from grid
4. **ID Calculation**: Convert binary to decimal marker ID
5. **Validation**: Verify marker format and checksum

#### 4.4 TUIO Bridge
**Responsibility**: Convert marker data to TUIO protocol
```cpp
class TUIOBridge {
public:
    bool initialize(const std::string& host, int port);
    void updateMarkers(const std::vector<DecodedMarker>& markers);
    void removeMarker(int sessionId);
    void commitFrame();
    
private:
    std::unique_ptr<TuioServer> tuioServer;
    std::map<int, TuioObject*> activeMarkers;
    long nextSessionId;
};
```

**TUIO Integration**:
- Map Codice marker IDs to TUIO symbol IDs
- Maintain session IDs for marker tracking
- Handle marker lifecycle (add/update/remove)
- Stream TUIO messages via UDP/TCP

### 5. Threading Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Main Thread    │    │ Capture Thread  │    │ Detection Thread│
│                 │    │                 │    │                 │
│ • GUI Events    │    │ • Camera I/O    │    │ • Image Proc    │
│ • User Input    │    │ • Frame Buffer  │    │ • Marker Detect │
│ • TUIO Output   │    │ • Error Handle  │    │ • ID Decode     │
│ • Settings      │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 ▼
                    ┌─────────────────┐
                    │  Shared Data    │
                    │                 │
                    │ • Frame Buffer  │
                    │ • Detection     │
                    │ • Settings      │
                    │ • Status        │
                    └─────────────────┘
```

**Thread Safety**:
- Mutex protection for shared data
- Lock-free queues for frame passing
- Atomic operations for status flags
- RAII for resource management

### 6. Configuration Architecture

```json
{
  "application": {
    "name": "Codice-Cam",
    "version": "1.0.0",
    "log_level": "INFO"
  },
  "camera": {
    "device_id": 0,
    "width": 1280,
    "height": 720,
    "fps": 30,
    "auto_exposure": true,
    "brightness": 0,
    "contrast": 0
  },
  "detection": {
    "min_marker_size": 40,
    "max_marker_size": 100,
    "detection_threshold": 0.7,
    "preprocessing": {
      "blur_kernel_size": 3,
      "threshold_type": "ADAPTIVE",
      "morphology_kernel_size": 3
    }
  },
  "tuio": {
    "host": "localhost",
    "port": 3333,
    "protocol": "UDP",
    "source_name": "Codice-Cam",
    "full_update_interval": 1.0
  },
  "gui": {
    "show_overlay": true,
    "overlay_alpha": 0.7,
    "window_width": 1024,
    "window_height": 768,
    "fullscreen": false
  }
}
```

### 7. Error Handling Architecture

```
┌─────────────────┐
│   Application   │
│     Layer       │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   Component     │
│     Layer       │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   System        │
│     Layer       │
└─────────────────┘
```

**Error Categories**:
- **Camera Errors**: Device not found, capture failure
- **Detection Errors**: Invalid markers, processing failure
- **TUIO Errors**: Network issues, protocol errors
- **System Errors**: Memory allocation, thread issues

**Error Handling Strategy**:
- Graceful degradation (continue with reduced functionality)
- User notification with actionable messages
- Automatic recovery where possible
- Comprehensive logging for debugging

### 8. Performance Architecture

#### 8.1 Optimization Strategies
```
┌─────────────────┐
│   Input         │
│   Optimization  │
├─────────────────┤
│ • Frame Skipping│
│ • ROI Processing│
│ • Resolution    │
│   Scaling       │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   Processing    │
│   Optimization  │
├─────────────────┤
│ • Multi-threading│
│ • SIMD Ops      │
│ • Memory Pool   │
│ • Algorithm     │
│   Tuning        │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   Output        │
│   Optimization  │
├─────────────────┤
│ • Batch Updates │
│ • Selective     │
│   Streaming     │
│ • Compression   │
└─────────────────┘
```

#### 8.2 Performance Monitoring
- Frame processing time
- Detection accuracy metrics
- Memory usage tracking
- TUIO stream latency
- CPU utilization

### 9. Security Considerations

#### 9.1 Network Security
- Local network only (no internet exposure)
- Configurable port binding
- Input validation for network parameters

#### 9.2 Data Privacy
- No video recording or storage
- Local processing only
- No data transmission beyond TUIO stream

### 10. Deployment Architecture

```
┌─────────────────┐
│   Development   │
│   Environment   │
├─────────────────┤
│ • Visual Studio │
│ • Debug Build   │
│ • Test Data     │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   Release       │
│   Environment   │
├─────────────────┤
│ • Optimized     │
│   Build         │
│ • Installer     │
│ • Documentation │
└─────────────────┘
         │
         ▼
┌─────────────────┐
│   Target        │
│   Environment   │
├─────────────────┤
│ • Windows 10/11 │
│ • Webcam        │
│ • MT Software   │
└─────────────────┘
```

---

**Document Status**: Draft  
**Last Updated**: January 2025  
**Review Cycle**: Major architecture changes
