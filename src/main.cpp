#include <iostream>
#include <opencv2/opencv.hpp>
#include <SDL2/SDL.h>

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
    
    // Test camera availability
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cout << "⚠️  No camera detected (this is normal in headless environments)" << std::endl;
    } else {
        std::cout << "📹 Camera detected and ready" << std::endl;
        cap.release();
    }
    
    std::cout << std::endl;
    std::cout << "🎯 Development environment is ready!" << std::endl;
    std::cout << "Next steps:" << std::endl;
    std::cout << "1. Implement camera capture module" << std::endl;
    std::cout << "2. Develop marker detection algorithm" << std::endl;
    std::cout << "3. Integrate TUIO protocol" << std::endl;
    std::cout << "4. Create user interface" << std::endl;
    
    SDL_Quit();
    return 0;
}
