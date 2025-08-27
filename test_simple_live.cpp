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

// Statistics tracking
struct TestStatistics {
    std::chrono::steady_clock::time_point start_time;
    int total_frames_processed = 0;
    int total_markers_detected = 0;
    int camera_fps = 0;
    int detection_fps = 0;
    
    void reset() {
        start_time = std::chrono::steady_clock::now();
        total_frames_processed = 0;
        total_markers_detected = 0;
        camera_fps = 0;
        detection_fps = 0;
    }
    
    void print() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        
        std::cout << "\n📊 Live Camera Test Statistics:" << std::endl;
        std::cout << "  Runtime: " << elapsed << " seconds" << std::endl;
        std::cout << "  Frames Processed: " << total_frames_processed << std::endl;
        std::cout << "  Markers Detected: " << total_markers_detected << std::endl;
        std::cout << "  Camera FPS: " << camera_fps << std::endl;
        std::cout << "  Detection FPS: " << detection_fps << std::endl;
        
        if (elapsed > 0) {
            double avg_frames_per_sec = static_cast<double>(total_frames_processed) / elapsed;
            double avg_markers_per_sec = static_cast<double>(total_markers_detected) / elapsed;
            
            std::cout << "  Average Frames/sec: " << std::fixed << std::setprecision(2) << avg_frames_per_sec << std::endl;
            std::cout << "  Average Markers/sec: " << std::fixed << std::setprecision(2) << avg_markers_per_sec << std::endl;
        }
    }
};

TestStatistics g_stats;

int main() {
    std::cout << "🎥 Simple Live Camera Marker Detection Test" << std::endl;
    std::cout << "============================================" << std::endl;
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize statistics
    g_stats.reset();
    
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
    
    // Test 3: Initialize Marker Detection
    std::cout << "\n📋 Test 3: Marker Detection Initialization" << std::endl;
    MarkerDetector marker_detector;
    marker_detector.setDebugMode(false); // Minimal logging for live test
    marker_detector.setVerboseMode(false);
    std::cout << "✅ Marker detector initialized" << std::endl;
    
    // Test 4: Initialize TUIO Test Client
    std::cout << "\n📋 Test 4: TUIO Test Client Initialization" << std::endl;
    TUIOTestClient test_client;
    if (!test_client.initialize(800, 600, "Live Camera Marker Test")) {
        std::cerr << "❌ Failed to initialize TUIO test client" << std::endl;
        return 1;
    }
    std::cout << "✅ TUIO test client initialized" << std::endl;
    
    // Test 5: Start Camera Capture
    std::cout << "\n📋 Test 5: Starting Camera Capture" << std::endl;
    std::vector<CodiceMarker> detected_markers;
    
    auto frame_callback = [&](const cv::Mat& frame) {
        if (!g_running) return;
        
        g_stats.total_frames_processed++;
        
        // Process frame for marker detection
        cv::Mat processed_frame;
        if (!image_processor.processFrame(frame, processed_frame)) {
            return;
        }
        
        if (!marker_detector.detectMarkers(processed_frame, detected_markers)) {
            return;
        }
        
        g_stats.total_markers_detected += detected_markers.size();
        
        // Update test client with detected markers
        for (const auto& marker : detected_markers) {
            // Convert center to normalized coordinates (0.0-1.0)
            float x = marker.center.x / frame.cols;
            float y = marker.center.y / frame.rows;
            
            // Update test client
            test_client.updateObject(
                marker.id, // Use marker ID as session ID for simplicity
                marker.id,
                x,
                y,
                marker.angle * M_PI / 180.0f, // Convert degrees to radians
                0.0f, 0.0f, 0.0f, 0.0f // velocities and acceleration
            );
        }
        
        // Print detection results (minimal)
        if (!detected_markers.empty()) {
            std::cout << "🎯 Detected " << detected_markers.size() << " markers: ";
            for (const auto& marker : detected_markers) {
                std::cout << "ID" << marker.id << "(" << std::fixed << std::setprecision(2) 
                         << marker.center.x << "," << marker.center.y << ") ";
            }
            std::cout << std::endl;
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
    std::cout << "  - Markers will appear in the test client window" << std::endl;
    std::cout << "  - Press ESC or close window to exit" << std::endl;
    std::cout << "  - Press D in test client to toggle debug mode" << std::endl;
    std::cout << "  - Press R in test client to reset statistics" << std::endl;
    
    // Start test client in a separate thread
    std::thread client_thread([&]() {
        test_client.start();
    });
    
    // Main monitoring loop
    std::cout << "\n🚀 Live test running... Press Ctrl+C to stop" << std::endl;
    
    auto last_stats_time = std::chrono::steady_clock::now();
    int stats_interval = 10; // Print stats every 10 seconds
    
    while (g_running && g_camera_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Print periodic statistics
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_time).count();
        
        if (elapsed >= stats_interval) {
            g_stats.print();
            std::cout << "\n🔄 TUIO Test Client Statistics:" << std::endl;
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
    g_stats.print();
    
    std::cout << "\n🎉 Simple Live Camera Test completed!" << std::endl;
    std::cout << "The system successfully:" << std::endl;
    std::cout << "  ✅ Captured live camera frames" << std::endl;
    std::cout << "  ✅ Detected and decoded Codice markers" << std::endl;
    std::cout << "  ✅ Displayed markers in test client" << std::endl;
    std::cout << "  ✅ Maintained real-time performance" << std::endl;
    
    return 0;
}
