#include "include/CameraManager.h"
#include "include/ImageProcessor.h"
#include "include/MarkerDetector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <iomanip>

using namespace CodiceCam;

// Global variables for signal handling
std::atomic<bool> g_running(true);

// Configuration structure
struct DetectionConfig {
    // Camera settings
    int camera_width = 1920;
    int camera_height = 1080;
    int camera_fps = 15;

    // Image processing
    int blur_kernel_size = 1;
    double contrast_alpha = 1.3;
    int brightness_beta = 20;

    // Edge detection
    int canny_low_threshold = 30;
    int canny_high_threshold = 100;

    // Contour filtering
    double min_contour_area = 500;
    double max_contour_area = 100000;
    double min_contour_perimeter = 80;

    // Marker validation
    int min_marker_size = 30;
    int max_marker_size = 300;
    double min_confidence = 0.6;

    // Debug options
    bool debug_mode = true;
    bool verbose_mode = false;
};

// Simple config file parser
DetectionConfig loadConfig(const std::string& filename) {
    DetectionConfig config;
    std::ifstream file(filename);
    std::string line;

    std::cout << "📋 Loading configuration from " << filename << "..." << std::endl;

    if (!file.is_open()) {
        std::cout << "⚠️  Config file not found, using default values" << std::endl;
        return config;
    }

    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        // Remove comments from value
        size_t comment_pos = value.find('#');
        if (comment_pos != std::string::npos) {
            value = value.substr(0, comment_pos);
        }

        // Remove whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Parse values
        if (key == "camera_width") config.camera_width = std::stoi(value);
        else if (key == "camera_height") config.camera_height = std::stoi(value);
        else if (key == "camera_fps") config.camera_fps = std::stoi(value);
        else if (key == "blur_kernel_size") config.blur_kernel_size = std::stoi(value);
        else if (key == "contrast_alpha") config.contrast_alpha = std::stod(value);
        else if (key == "brightness_beta") config.brightness_beta = std::stoi(value);
        else if (key == "canny_low_threshold") config.canny_low_threshold = std::stoi(value);
        else if (key == "canny_high_threshold") config.canny_high_threshold = std::stoi(value);
        else if (key == "min_contour_area") config.min_contour_area = std::stod(value);
        else if (key == "max_contour_area") config.max_contour_area = std::stod(value);
        else if (key == "min_contour_perimeter") config.min_contour_perimeter = std::stod(value);
        else if (key == "min_marker_size") config.min_marker_size = std::stoi(value);
        else if (key == "max_marker_size") config.max_marker_size = std::stoi(value);
        else if (key == "min_confidence") config.min_confidence = std::stod(value);
        else if (key == "debug_mode") config.debug_mode = (value == "true");
        else if (key == "verbose_mode") config.verbose_mode = (value == "true");
    }

    std::cout << "✅ Configuration loaded successfully" << std::endl;
    return config;
}

void printConfig(const DetectionConfig& config) {
    std::cout << "\n📋 Current Detection Configuration:" << std::endl;
    std::cout << "  Camera Settings:" << std::endl;
    std::cout << "    - Resolution: " << config.camera_width << "x" << config.camera_height << std::endl;
    std::cout << "    - Target FPS: " << config.camera_fps << std::endl;
    std::cout << "  Image Processing:" << std::endl;
    std::cout << "    - Blur kernel: " << config.blur_kernel_size << std::endl;
    std::cout << "    - Contrast: " << config.contrast_alpha << std::endl;
    std::cout << "    - Brightness: " << config.brightness_beta << std::endl;
    std::cout << "  Edge Detection:" << std::endl;
    std::cout << "    - Low threshold: " << config.canny_low_threshold << std::endl;
    std::cout << "    - High threshold: " << config.canny_high_threshold << std::endl;
    std::cout << "  Contour Filtering:" << std::endl;
    std::cout << "    - Area range: " << config.min_contour_area << " - " << config.max_contour_area << std::endl;
    std::cout << "    - Min perimeter: " << config.min_contour_perimeter << std::endl;
    std::cout << "  Marker Validation:" << std::endl;
    std::cout << "    - Size range: " << config.min_marker_size << " - " << config.max_marker_size << std::endl;
    std::cout << "    - Min confidence: " << config.min_confidence << std::endl;
    std::cout << "  Debug Options:" << std::endl;
    std::cout << "    - Debug mode: " << (config.debug_mode ? "ON" : "OFF") << std::endl;
    std::cout << "    - Verbose mode: " << (config.verbose_mode ? "ON" : "OFF") << std::endl;
}

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ", shutting down gracefully..." << std::endl;
    g_running = false;
}

int main() {
    std::cout << "🎥 Configurable Marker Detection Test" << std::endl;
    std::cout << "=====================================" << std::endl;

    // Load configuration
    DetectionConfig config = loadConfig("detection_config.txt");
    printConfig(config);

    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize Camera with config resolution
    std::cout << "\n📋 Initializing Camera..." << std::endl;
    std::cout << "📐 Using resolution: " << config.camera_width << "x" << config.camera_height << " @ " << config.camera_fps << " FPS" << std::endl;
    CameraManager camera(2, config.camera_width, config.camera_height); // Use camera ID 2 with configured resolution
    if (!camera.initialize()) {
        std::cerr << "❌ Failed to initialize camera" << std::endl;
        return 1;
    }
    std::cout << "✅ Camera initialized successfully" << std::endl;

    // Initialize Image Processing with config values
    std::cout << "\n📋 Initializing Image Processing..." << std::endl;
    ImageProcessor image_processor;
    image_processor.setPreprocessingParams(config.blur_kernel_size, config.contrast_alpha, config.brightness_beta);
    image_processor.setEdgeDetectionParams(config.canny_low_threshold, config.canny_high_threshold);
    image_processor.setContourFilterParams(config.min_contour_area, config.max_contour_area, config.min_contour_perimeter);
    std::cout << "✅ Image processor initialized with custom settings" << std::endl;

    // Initialize Marker Detection with config values
    std::cout << "\n📋 Initializing Marker Detection..." << std::endl;
    MarkerDetector marker_detector;
    marker_detector.setDetectionParams(config.min_marker_size, config.max_marker_size, config.min_confidence);
    marker_detector.setDebugMode(config.debug_mode);
    marker_detector.setVerboseMode(config.verbose_mode);
    std::cout << "✅ Marker detector initialized with custom settings" << std::endl;

    // Clear debug output folder if debug mode is enabled
    if (config.debug_mode) {
        std::cout << "\n🧹 Clearing debug output folder..." << std::endl;
        int result = system("rm -rf debug_output/* 2>/dev/null");
        if (result == 0) {
            std::cout << "✅ Debug output folder cleared" << std::endl;
        } else {
            std::cout << "⚠️ Debug output folder may not exist (will be created automatically)" << std::endl;
        }

        // Ensure debug_output directory exists
        result = system("mkdir -p debug_output 2>/dev/null");
        if (result != 0) {
            std::cerr << "❌ Failed to create debug_output directory" << std::endl;
        }
    }

    // Start Camera Capture
    std::cout << "\n📋 Starting Camera Capture..." << std::endl;
    std::vector<CodiceMarker> detected_markers;
    int frame_count = 0;
    int marker_detection_count = 0;
    int processed_frame_count = 0;

    auto frame_callback = [&](const cv::Mat& frame) {
        if (!g_running) return;

        frame_count++;

        // Process every 10th frame to reduce processing load
        if (frame_count % 10 != 0) return;

        processed_frame_count++;

        // Process frame for marker detection
        cv::Mat processed_frame;
        if (!image_processor.processFrame(frame, processed_frame)) {
            std::cout << "❌ Image processing failed on frame " << frame_count << std::endl;
            return;
        }

        if (!marker_detector.detectMarkers(frame, processed_frame, detected_markers)) {
            std::cout << "❌ Marker detection failed on frame " << frame_count << std::endl;
            return;
        }

        // Report findings
        if (!detected_markers.empty()) {
            marker_detection_count++;
            std::cout << "\n🎯 MARKERS DETECTED! Frame " << frame_count << " (processed #" << processed_frame_count << ")" << std::endl;
            for (const auto& marker : detected_markers) {
                std::cout << "  📍 Marker ID: " << marker.id
                          << " | Center: (" << std::fixed << std::setprecision(1)
                          << marker.center.x << "," << marker.center.y << ")"
                          << " | Angle: " << marker.angle << "°"
                          << " | Deskew: " << marker.deskew_angle << "°"
                          << " | Confidence: " << std::setprecision(2) << marker.confidence
                          << std::endl;
            }
        } else if (processed_frame_count % 30 == 0) { // Report status every ~10 seconds at 30fps
            std::cout << "📊 Processed frame " << processed_frame_count << " (total " << frame_count << "), no markers detected" << std::endl;
        }
    };

    if (!camera.startCapture(frame_callback)) {
        std::cerr << "❌ Failed to start camera capture" << std::endl;
        return 1;
    }
    std::cout << "✅ Camera capture started" << std::endl;

    // Instructions
    std::cout << "\n📋 Instructions:" << std::endl;
    std::cout << "  - Show Codice markers to the camera" << std::endl;
    std::cout << "  - Debug images will be saved to debug_output/ folder to show detection attempts" << std::endl;
    std::cout << "  - Edit detection_config.txt to adjust sensitivity" << std::endl;
    std::cout << "  - Press Ctrl+C to exit" << std::endl;

    // Main monitoring loop
    std::cout << "\n🚀 Test running... Press Ctrl+C to stop" << std::endl;

    auto start_time = std::chrono::steady_clock::now();
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Print status every 10 seconds
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        if (elapsed % 10 == 0 && elapsed > 0) {
            std::cout << "📊 Status: " << frame_count << " frames captured, "
                      << processed_frame_count << " processed, "
                      << marker_detection_count << " marker detections" << std::endl;
        }
    }

    // Cleanup
    std::cout << "\n🛑 Shutting down..." << std::endl;
    camera.stopCapture();
    std::cout << "✅ Camera stopped" << std::endl;

    std::cout << "\n🎉 Test completed!" << std::endl;
    std::cout << "📊 Final Statistics:" << std::endl;
    std::cout << "  - Total frames captured: " << frame_count << std::endl;
    std::cout << "  - Frames processed: " << processed_frame_count << std::endl;
    std::cout << "  - Total marker detections: " << marker_detection_count << std::endl;
    std::cout << "  - Detection rate: " << (processed_frame_count > 0 ? (double)marker_detection_count/processed_frame_count*100 : 0) << "%" << std::endl;

    std::cout << "\n💡 Tuning Tips:" << std::endl;
    std::cout << "  - Check debug_frame.jpg to see what the system is detecting" << std::endl;
    std::cout << "  - Yellow outlines = 4-corner candidates (potential markers)" << std::endl;
    std::cout << "  - Orange outlines = multi-corner shapes (noisy detection)" << std::endl;
    std::cout << "  - Green outlines = successfully validated markers" << std::endl;
    std::cout << "  - Adjust parameters in detection_config.txt and rerun" << std::endl;

    return 0;
}
