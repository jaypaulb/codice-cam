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
    std::cout << "🎥 Debug Live Camera Marker Detection Test" << std::endl;
    std::cout << "===========================================" << std::endl;
    
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
    
    // Test 3: Initialize Marker Detection
    std::cout << "\n📋 Test 3: Marker Detection Initialization" << std::endl;
    MarkerDetector marker_detector;
    marker_detector.setDebugMode(true); // Enable debug mode for troubleshooting
    marker_detector.setVerboseMode(true);
    std::cout << "✅ Marker detector initialized with debug mode" << std::endl;
    
    // Test 4: Initialize TUIO Test Client
    std::cout << "\n📋 Test 4: TUIO Test Client Initialization" << std::endl;
    TUIOTestClient test_client;
    if (!test_client.initialize(800, 600, "Debug Live Camera Marker Test")) {
        std::cerr << "❌ Failed to initialize TUIO test client" << std::endl;
        return 1;
    }
    std::cout << "✅ TUIO test client initialized" << std::endl;
    
    // Test 5: Start Camera Capture
    std::cout << "\n📋 Test 5: Starting Camera Capture" << std::endl;
    std::vector<CodiceMarker> detected_markers;
    int frame_count = 0;
    
    auto frame_callback = [&](const cv::Mat& frame) {
        if (!g_running) return;
        
        frame_count++;
        
        // Print frame info every 30 frames (about once per second at 30fps)
        if (frame_count % 30 == 0) {
            std::cout << "📷 Frame " << frame_count << " received: " 
                      << frame.cols << "x" << frame.rows << " channels=" << frame.channels() << std::endl;
        }
        
        // Process frame for marker detection
        cv::Mat processed_frame;
        if (!image_processor.processFrame(frame, processed_frame)) {
            if (frame_count % 30 == 0) {
                std::cout << "❌ Image processing failed on frame " << frame_count << std::endl;
            }
            return;
        }
        
        if (frame_count % 30 == 0) {
            std::cout << "✅ Image processing successful on frame " << frame_count << std::endl;
        }
        
        if (!marker_detector.detectMarkers(processed_frame, detected_markers)) {
            if (frame_count % 30 == 0) {
                std::cout << "❌ Marker detection failed on frame " << frame_count << std::endl;
            }
            return;
        }
        
        if (frame_count % 30 == 0) {
            std::cout << "✅ Marker detection successful on frame " << frame_count 
                      << ", found " << detected_markers.size() << " markers" << std::endl;
        }
        
        // Update test client with detected markers
        for (const auto& marker : detected_markers) {
            // Convert center to normalized coordinates (0.0-1.0)
            float x = marker.center.x / frame.cols;
            float y = marker.center.y / frame.rows;
            
            std::cout << "🎯 Marker ID " << marker.id << " at (" 
                      << std::fixed << std::setprecision(2) 
                      << marker.center.x << "," << marker.center.y << ") -> normalized (" 
                      << x << "," << y << ")" << std::endl;
            
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
    std::cout << "  - Watch console for debug output" << std::endl;
    std::cout << "  - Markers will appear in the test client window" << std::endl;
    std::cout << "  - Press ESC or close window to exit" << std::endl;
    
    // Start test client in a separate thread
    std::thread client_thread([&]() {
        test_client.start();
    });
    
    // Main monitoring loop
    std::cout << "\n🚀 Debug test running... Press Ctrl+C to stop" << std::endl;
    std::cout << "📊 Monitoring frame processing..." << std::endl;
    
    while (g_running && g_camera_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Print status every 5 seconds
        std::cout << "📊 Status: " << frame_count << " frames processed" << std::endl;
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
    
    std::cout << "\n🎉 Debug test completed!" << std::endl;
    std::cout << "Total frames processed: " << frame_count << std::endl;
    
    return 0;
}
