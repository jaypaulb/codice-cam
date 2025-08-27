#include <iostream>
#include <opencv2/opencv.hpp>
#include <SDL2/SDL.h>
#include "CameraManager.h"
#include "MarkerDetector.h"
#include <chrono>
#include <thread>
#include <iomanip>

using namespace CodiceCam;

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool test_mode = false;
    bool debug_mode = false;
    bool verbose_mode = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--test") {
            test_mode = true;
        } else if (arg == "--debug") {
            debug_mode = true;
        } else if (arg == "--verbose") {
            verbose_mode = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --test     Test mode using saved gray_marker.jpg" << std::endl;
            std::cout << "  --debug    Full debug logging" << std::endl;
            std::cout << "  --verbose  Medium logging" << std::endl;
            std::cout << "  --help     Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "Logging levels:" << std::endl;
            std::cout << "  No option: Minimal - just detected marker info" << std::endl;
            std::cout << "  --verbose: Medium - key detection steps" << std::endl;
            std::cout << "  --debug:   Full - all debug information" << std::endl;
            return 0;
        }
    }

    if (!test_mode) {
        std::cout << "🚀 Codice-Cam v1.0.0" << std::endl;
        std::cout << "Webcam-based Codice Marker Detection for TUIO" << std::endl;
        if (debug_mode) {
            std::cout << "🐛 Debug mode enabled" << std::endl;
        } else if (verbose_mode) {
            std::cout << "📝 Verbose mode enabled" << std::endl;
        } else {
            std::cout << "🔇 Minimal logging mode" << std::endl;
        }
        std::cout << std::endl;
    }

    // Check for test mode
    if (test_mode) {
        if (debug_mode || verbose_mode) {
            std::cout << "🧪 Running in TEST MODE - using saved gray_marker.jpg" << std::endl;
            std::cout << std::endl;
        }

        // Load the saved marker image
        cv::Mat test_marker = cv::imread("gray_marker.jpg", cv::IMREAD_GRAYSCALE);
        if (test_marker.empty()) {
            std::cerr << "❌ Could not load gray_marker.jpg for testing" << std::endl;
            return 1;
        }

        if (debug_mode || verbose_mode) {
            std::cout << "📸 Loaded test marker image: " << test_marker.cols << "x" << test_marker.rows << std::endl;
        }

        // Create marker detector
        MarkerDetector detector;
        detector.setDebugMode(debug_mode || verbose_mode);  // Enable debug for test mode if verbose or debug
        detector.setDetectionParams(30, 300, 0.6);

        if (debug_mode || verbose_mode) {
            std::cout << "🔍 Testing pattern validation..." << std::endl;
        }

        // Convert to binary
        cv::Mat binary_marker;
        cv::threshold(test_marker, binary_marker, 127, 255, cv::THRESH_BINARY);
        if (debug_mode) {
            cv::imwrite("test_binary_marker.jpg", binary_marker);
            std::cout << "💾 Saved test binary marker to test_binary_marker.jpg" << std::endl;
        }

        // Test validation
        int marker_id;
        double confidence;
        if (detector.testDecodeMarker(test_marker, marker_id, confidence)) {
            std::cout << "✅ SUCCESS! Marker ID: " << marker_id << ", Confidence: " << confidence << std::endl;
        } else {
            std::cout << "❌ Pattern validation failed" << std::endl;
        }

        return 0;
    }

    // Test OpenCV
    if (debug_mode || verbose_mode) {
        std::cout << "📷 OpenCV Version: " << CV_VERSION << std::endl;
    }

    // Test SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "❌ SDL2 initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (debug_mode || verbose_mode) {
        std::cout << "✅ SDL2 initialized successfully" << std::endl;
        std::cout << std::endl;
        std::cout << "🎥 Initializing CameraManager and MarkerDetector..." << std::endl;
    }

    // Use camera device 2 (we know this is your webcam)
    int working_camera_id = 2;
    if (debug_mode || verbose_mode) {
        std::cout << "📹 Using camera device " << working_camera_id << " (your webcam)" << std::endl;
    }

    CameraManager camera(working_camera_id, 640, 480);
    MarkerDetector detector;

    if (!camera.initialize()) {
        std::cerr << "❌ Failed to initialize camera" << std::endl;
        SDL_Quit();
        return -1;
    }

    if (debug_mode || verbose_mode) {
        std::cout << "📹 Camera initialized successfully" << std::endl;
        std::cout << "📐 Frame size: " << camera.getFrameSize().width << "x" << camera.getFrameSize().height << std::endl;
    }

    // Configure marker detection based on logging level
    if (debug_mode || verbose_mode) {
        std::cout << "🔍 Enabling marker detection..." << std::endl;
    }
    detector.setDebugMode(debug_mode);  // Only enable debug mode if explicitly requested
    detector.setVerboseMode(verbose_mode);
    detector.setDetectionParams(30, 300, 0.6);  // More lenient parameters for testing

    if (debug_mode || verbose_mode) {
        std::cout << "✅ Marker detection initialized" << std::endl;
    }

    // Test frame capture with marker detection
    int frame_count = 0;
    int markers_detected = 0;
    auto start_time = std::chrono::steady_clock::now();

    auto frame_callback = [&frame_count, &markers_detected, &start_time, &detector, debug_mode, verbose_mode](const cv::Mat& frame) {
        frame_count++;

        if (frame_count == 1 && (debug_mode || verbose_mode)) {
            std::cout << "📸 First frame received, starting marker detection..." << std::endl;
        }

        // Detect markers in the frame
        std::vector<CodiceMarker> markers;
        if (detector.detectMarkers(frame, markers)) {
            markers_detected += markers.size();

            // Log detected markers based on logging level
            for (const auto& marker : markers) {
                if (debug_mode || verbose_mode) {
                    std::cout << "🎯 Detected marker ID: " << marker.id
                              << ", at location (" << std::fixed << std::setprecision(1)
                              << marker.center.x << ", " << marker.center.y << ")"
                              << ", confidence: " << std::setprecision(2) << marker.confidence << std::endl;
                } else {
                    // Minimal logging - just the essential info
                    std::cout << "Detected marker ID " << marker.id
                              << ", at location " << std::fixed << std::setprecision(0)
                              << marker.center.x << ", " << marker.center.y << std::endl;
                }
            }
        }

        // Print frame info every 30 frames (about 1 second at 30fps) - only in verbose/debug modes
        if ((debug_mode || verbose_mode) && frame_count % 30 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
            double fps = (frame_count * 1000.0) / elapsed;

            std::cout << "📊 Frames: " << frame_count
                      << ", FPS: " << std::fixed << std::setprecision(1) << fps
                      << ", Markers: " << markers_detected << std::endl;
        }

        // Stop after 10 seconds of capture (300 frames at 30fps = 10 seconds)
        if (frame_count >= 300) {
            return;
        }
    };

    if (!camera.startCapture(frame_callback)) {
        std::cerr << "❌ Failed to start camera capture" << std::endl;
        SDL_Quit();
        return -1;
    }

    // Wait for capture to complete
    while (camera.isCapturing() && frame_count < 300) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    camera.stopCapture();
    detector.setDebugMode(false);

    if (debug_mode || verbose_mode) {
        std::cout << std::endl;
        std::cout << "🎯 Marker detection test completed!" << std::endl;
        std::cout << "✅ Total frames processed: " << frame_count << std::endl;
        std::cout << "✅ Total markers detected: " << markers_detected << std::endl;
        std::cout << std::endl;
        std::cout << detector.getDetectionStats() << std::endl;
        std::cout << std::endl;
        std::cout << "Next steps:" << std::endl;
        std::cout << "1. ✅ Camera capture module implemented" << std::endl;
        std::cout << "2. ✅ Marker detection algorithm implemented" << std::endl;
        std::cout << "3. Integrate TUIO protocol" << std::endl;
        std::cout << "4. Create user interface" << std::endl;
    } else {
        // Minimal mode - just show final summary
        std::cout << std::endl;
        std::cout << "Detection completed. Total markers detected: " << markers_detected << std::endl;
    }

    SDL_Quit();
    return 0;
}
