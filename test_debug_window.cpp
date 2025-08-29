#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>

int main() {
    std::cout << "🖥️ Testing debug window functionality..." << std::endl;
    
    // Initialize camera
    cv::VideoCapture cap(2); // Use camera device 2
    if (!cap.isOpened()) {
        std::cerr << "❌ Failed to open camera" << std::endl;
        return -1;
    }
    
    // Set camera properties
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    cap.set(cv::CAP_PROP_FPS, 30);
    
    std::cout << "📹 Camera opened successfully" << std::endl;
    
    // Create debug window
    cv::namedWindow("Debug Window Test", cv::WINDOW_AUTOSIZE);
    
    int frame_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (frame_count < 150) { // Run for 5 seconds
        cv::Mat frame;
        if (!cap.read(frame)) {
            std::cerr << "❌ Failed to read frame" << std::endl;
            break;
        }
        
        if (frame.empty()) {
            std::cerr << "⚠️ Empty frame" << std::endl;
            continue;
        }
        
        frame_count++;
        
        // Add some debug info to the frame
        std::string info = "Frame: " + std::to_string(frame_count);
        cv::putText(frame, info, cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        
        // Show frame
        cv::imshow("Debug Window Test", frame);
        
        // Non-blocking wait
        int key = cv::waitKey(1);
        if (key == 27) { // ESC key
            std::cout << "🖥️ ESC pressed, exiting..." << std::endl;
            break;
        }
        
        // Print progress every 30 frames
        if (frame_count % 30 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
            double fps = (frame_count * 1000.0) / elapsed;
            std::cout << "📊 Frame: " << frame_count << ", FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
        }
    }
    
    std::cout << "✅ Test completed successfully!" << std::endl;
    
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
