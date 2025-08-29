#include "include/CameraManager.h"
#include "include/ImageProcessor.h"
#include "include/MarkerDetector.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <iomanip>

using namespace CodiceCam;

// Global variables for signal handling
std::atomic<bool> g_running(true);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ", shutting down gracefully..." << std::endl;
    g_running = false;
}

int main() {
    std::cout << "🎥 Simple Marker Detection Test" << std::endl;
    std::cout << "===============================" << std::endl;

    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize Camera
    std::cout << "\n📋 Initializing Camera..." << std::endl;
    CameraManager camera(2); // Use camera ID 2
    if (!camera.initialize()) {
        std::cerr << "❌ Failed to initialize camera" << std::endl;
        return 1;
    }
    std::cout << "✅ Camera initialized successfully" << std::endl;

    // Initialize Image Processing
    std::cout << "\n📋 Initializing Image Processing..." << std::endl;
    ImageProcessor image_processor;
    std::cout << "✅ Image processor initialized" << std::endl;

    // Initialize Marker Detection
    std::cout << "\n📋 Initializing Marker Detection..." << std::endl;
    MarkerDetector marker_detector;
    marker_detector.setDebugMode(true); // Enable debug mode for troubleshooting
    marker_detector.setVerboseMode(false); // Disable verbose to reduce clutter
    std::cout << "✅ Marker detector initialized with debug mode" << std::endl;

    // Clear debug output folder
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

    // Start Camera Capture
    std::cout << "\n📋 Starting Camera Capture..." << std::endl;
    std::vector<CodiceMarker> detected_markers;
    int frame_count = 0;
    int marker_detection_count = 0;

    auto frame_callback = [&](const cv::Mat& frame) {
        if (!g_running) return;

        frame_count++;

        // Process every 10th frame to reduce processing load
        if (frame_count % 10 != 0) return;

        // Process frame for marker detection
        cv::Mat processed_frame;
        if (!image_processor.processFrame(frame, processed_frame)) {
            return;
        }

        if (!marker_detector.detectMarkers(frame, processed_frame, detected_markers)) {
            return;
        }

        // Report findings
        if (!detected_markers.empty()) {
            marker_detection_count++;
            std::cout << "\n🎯 MARKERS DETECTED! Frame " << frame_count << std::endl;
            for (const auto& marker : detected_markers) {
                std::cout << "  📍 Marker ID: " << marker.id
                          << " | Center: (" << std::fixed << std::setprecision(1)
                          << marker.center.x << "," << marker.center.y << ")"
                          << " | Angle: " << marker.angle << "°"
                          << " | Deskew: " << marker.deskew_angle << "°"
                          << " | Confidence: " << std::setprecision(2) << marker.confidence
                          << std::endl;
            }
        } else if (frame_count % 300 == 0) { // Report status every ~10 seconds at 30fps
            std::cout << "📊 Frame " << frame_count << " processed, no markers detected" << std::endl;
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
    std::cout << "  - Debug images will be saved to debug_output/ folder when markers are detected" << std::endl;
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
            std::cout << "📊 Status: " << frame_count << " frames processed, "
                      << marker_detection_count << " marker detections" << std::endl;
        }
    }

    // Cleanup
    std::cout << "\n🛑 Shutting down..." << std::endl;
    camera.stopCapture();
    std::cout << "✅ Camera stopped" << std::endl;

    std::cout << "\n🎉 Test completed!" << std::endl;
    std::cout << "📊 Final Statistics:" << std::endl;
    std::cout << "  - Total frames processed: " << frame_count << std::endl;
    std::cout << "  - Total marker detections: " << marker_detection_count << std::endl;
    std::cout << "  - Detection rate: " << (frame_count > 0 ? (double)marker_detection_count/frame_count*100 : 0) << "%" << std::endl;

    return 0;
}
