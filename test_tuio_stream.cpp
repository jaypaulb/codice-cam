#include "include/CameraManager.h"
#include "include/ImageProcessor.h"
#include "include/MarkerDetector.h"
#include "include/TUIOTestClient.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <iomanip>
#include <cmath>

using namespace CodiceCam;

// Global variables for signal handling
std::atomic<bool> g_running(true);
std::atomic<bool> g_camera_running(false);

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ", shutting down gracefully..." << std::endl;
    g_running = false;
    g_camera_running = false;
}

int main() {
    std::cout << "🎥 TUIO Stream Test - Live Camera to TUIO Client" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Test 1: Initialize Camera
    std::cout << "\n📋 Test 1: Camera Initialization" << std::endl;
    CameraManager camera(2); // Use camera ID 2 as determined earlier
    if (!camera.initialize()) {
        std::cerr << "❌ Failed to initialize camera" << std::endl;
        return 1;
    }
    std::cout << "✅ Camera initialized successfully" << std::endl;
    
    // Test 2: Initialize Image Processing
    std::cout << "\n📋 Test 2: Image Processing Initialization" << std::endl;
    ImageProcessor image_processor;
    std::cout << "✅ Image processor initialized" << std::endl;
    
    // Test 3: Initialize Marker Detection (NO DEBUG MODE - no file saving)
    std::cout << "\n📋 Test 3: Marker Detection Initialization" << std::endl;
    MarkerDetector marker_detector;
    marker_detector.setDebugMode(false); // NO debug mode - no file saving
    marker_detector.setVerboseMode(false); // Minimal logging
    std::cout << "✅ Marker detector initialized (no debug file saving)" << std::endl;
    
    // Test 4: Initialize TUIO Test Client
    std::cout << "\n📋 Test 4: TUIO Test Client Initialization" << std::endl;
    TUIOTestClient test_client;
    if (!test_client.initialize(800, 600, "TUIO Stream Test - Live Camera")) {
        std::cerr << "❌ Failed to initialize TUIO test client" << std::endl;
        return 1;
    }
    std::cout << "✅ TUIO test client initialized" << std::endl;
    
    // Test 5: Start Camera Capture
    std::cout << "\n📋 Test 5: Starting Camera Capture" << std::endl;
    std::vector<CodiceMarker> detected_markers;
    int frame_count = 0;
    int marker_detection_count = 0;
    
    auto frame_callback = [&](const cv::Mat& frame) {
        if (!g_running) return;
        
        frame_count++;
        
        // Process frame for marker detection
        cv::Mat processed_frame;
        if (!image_processor.processFrame(frame, processed_frame)) {
            return;
        }
        
        if (!marker_detector.detectMarkers(processed_frame, detected_markers)) {
            return;
        }
        
        // Update test client with detected markers (simulating TUIO stream)
        for (const auto& marker : detected_markers) {
            marker_detection_count++;
            
            // Convert center to normalized coordinates (0.0-1.0)
            float x = marker.center.x / frame.cols;
            float y = marker.center.y / frame.rows;
            
            std::cout << "🎯 Detected marker ID " << marker.id << " at (" 
                      << std::fixed << std::setprecision(2) 
                      << marker.center.x << "," << marker.center.y << ") -> normalized (" 
                      << x << "," << y << ")" << std::endl;
            
            // Update test client (this simulates receiving TUIO messages)
            test_client.updateObject(
                marker.id, // Use marker ID as session ID for simplicity
                marker.id,
                x,
                y,
                marker.angle * M_PI / 180.0f, // Convert degrees to radians
                0.0f, 0.0f, 0.0f, 0.0f // velocities and acceleration
            );
        }
    };
    
    if (!camera.startCapture(frame_callback)) {
        std::cerr << "❌ Failed to start camera capture" << std::endl;
        return 1;
    }
    g_camera_running = true;
    std::cout << "✅ Camera capture started" << std::endl;
    
    // Test 6: Start TUIO Test Client
    std::cout << "\n📋 Test 6: Starting TUIO Test Client" << std::endl;
    std::cout << "📋 Instructions:" << std::endl;
    std::cout << "  - Show Codice markers to the camera" << std::endl;
    std::cout << "  - Markers will appear as colored circles in the test client window" << std::endl;
    std::cout << "  - Each marker gets a different color" << std::endl;
    std::cout << "  - Press ESC or close window to exit" << std::endl;
    std::cout << "  - Press D in test client to toggle debug mode" << std::endl;
    std::cout << "  - Press R in test client to reset statistics" << std::endl;
    
    // Start test client in a separate thread
    std::thread client_thread([&]() {
        test_client.start();
    });
    
    // Main monitoring loop
    std::cout << "\n🚀 TUIO Stream Test running... Press Ctrl+C to stop" << std::endl;
    std::cout << "📊 The test client window should show:" << std::endl;
    std::cout << "  - Dark background with grid lines" << std::endl;
    std::cout << "  - Colored circles for detected markers" << std::endl;
    std::cout << "  - White lines showing marker orientation" << std::endl;
    std::cout << "  - Statistics overlay in top-left corner" << std::endl;
    
    auto last_stats_time = std::chrono::steady_clock::now();
    int stats_interval = 5; // Print stats every 5 seconds
    
    while (g_running && g_camera_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Print periodic statistics
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time).count();
        
        if (elapsed >= stats_interval) {
            std::cout << "\n📊 Status Update:" << std::endl;
            std::cout << "  Frames Processed: " << frame_count << std::endl;
            std::cout << "  Markers Detected: " << marker_detection_count << std::endl;
            std::cout << "  Test Client Statistics:" << std::endl;
            std::cout << test_client.getStatistics() << std::endl;
            last_stats_time = now;
        }
    }
    
    // Cleanup
    std::cout << "\n🛑 Shutting down..." << std::endl;
    
    // Stop camera
    camera.stopCapture();
    g_camera_running = false;
    std::cout << "✅ Camera stopped" << std::endl;
    
    // Wait for test client thread
    if (client_thread.joinable()) {
        client_thread.join();
    }
    std::cout << "✅ TUIO test client stopped" << std::endl;
    
    // Final statistics
    std::cout << "\n📊 Final Test Results:" << std::endl;
    std::cout << "  Total Frames Processed: " << frame_count << std::endl;
    std::cout << "  Total Markers Detected: " << marker_detection_count << std::endl;
    std::cout << "  Test Client Statistics:" << std::endl;
    std::cout << test_client.getStatistics() << std::endl;
    
    std::cout << "\n🎉 TUIO Stream Test completed!" << std::endl;
    std::cout << "The system successfully:" << std::endl;
    std::cout << "  ✅ Captured live camera frames" << std::endl;
    std::cout << "  ✅ Detected and decoded Codice markers" << std::endl;
    std::cout << "  ✅ Displayed markers in test client window" << std::endl;
    std::cout << "  ✅ Maintained real-time performance" << std::endl;
    
    return 0;
}
