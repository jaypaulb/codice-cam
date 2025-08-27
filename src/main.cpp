#include <iostream>
#include <opencv2/opencv.hpp>
#include <SDL2/SDL.h>
#include "CameraManager.h"
#include <chrono>
#include <thread>
#include <iomanip>

using namespace CodiceCam;

int main(int argc, char* argv[]) {
    std::cout << "🚀 Codice-Cam v1.0.0" << std::endl;
    std::cout << "Webcam-based Codice Marker Detection for TUIO" << std::endl;
    std::cout << std::endl;

    // Test OpenCV
    std::cout << "📷 OpenCV Version: " << CV_VERSION << std::endl;

    // Test SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "❌ SDL2 initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::cout << "✅ SDL2 initialized successfully" << std::endl;

    // Test CameraManager
    std::cout << std::endl;
    std::cout << "🎥 Testing CameraManager..." << std::endl;

    CameraManager camera(0, 640, 480);

    if (!camera.initialize()) {
        std::cerr << "❌ Failed to initialize camera" << std::endl;
        SDL_Quit();
        return -1;
    }

    std::cout << "📹 Camera initialized successfully" << std::endl;
    std::cout << "📐 Frame size: " << camera.getFrameSize().width << "x" << camera.getFrameSize().height << std::endl;

    // Test frame capture
    int frame_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    auto frame_callback = [&frame_count, &start_time](const cv::Mat& frame) {
        frame_count++;

        // Print frame info every 30 frames (about 1 second at 30fps)
        if (frame_count % 30 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
            double fps = (frame_count * 1000.0) / elapsed;

            std::cout << "📊 Frames captured: " << frame_count
                      << ", FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
        }

        // Stop after 5 seconds of capture (150 frames at 30fps = 5 seconds)
        if (frame_count >= 150) {
            return;
        }
    };

    if (!camera.startCapture(frame_callback)) {
        std::cerr << "❌ Failed to start camera capture" << std::endl;
        SDL_Quit();
        return -1;
    }

    // Wait for capture to complete
    while (camera.isCapturing() && frame_count < 150) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    camera.stopCapture();

    std::cout << std::endl;
    std::cout << "🎯 CameraManager test completed!" << std::endl;
    std::cout << "✅ Total frames captured: " << frame_count << std::endl;
    std::cout << std::endl;
    std::cout << "Next steps:" << std::endl;
    std::cout << "1. ✅ Camera capture module implemented" << std::endl;
    std::cout << "2. Develop marker detection algorithm" << std::endl;
    std::cout << "3. Integrate TUIO protocol" << std::endl;
    std::cout << "4. Create user interface" << std::endl;

    SDL_Quit();
    return 0;
}
