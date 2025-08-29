#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>
#include <thread>

int main() {
    std::cout << "🧪 Minimal Debug Window Test" << std::endl;
    std::cout << "Testing camera + contour detection + OpenCV window" << std::endl;

    // Open camera
    cv::VideoCapture cap(2);
    if (!cap.isOpened()) {
        std::cerr << "❌ Failed to open camera" << std::endl;
        return -1;
    }

    // Set camera properties
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    cap.set(cv::CAP_PROP_FPS, 30);

    std::cout << "📹 Camera opened successfully" << std::endl;

    // Create window
    cv::namedWindow("Minimal Debug Test", cv::WINDOW_AUTOSIZE);
    std::cout << "🖥️ Window created" << std::endl;

    int frame_count = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (frame_count < 300) { // 10 seconds at 30fps
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

        // Clone frame for processing
        cv::Mat display_frame = frame.clone();

        // Convert to color if needed
        if (display_frame.channels() == 1) {
            cv::cvtColor(display_frame, display_frame, cv::COLOR_GRAY2BGR);
        }

        // Simple contour detection (minimal version)
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Simple blur and threshold
        cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);
        cv::Mat binary;
        cv::threshold(gray, binary, 100, 255, cv::THRESH_BINARY);

        // Find contours
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // Draw contours in yellow
        for (size_t i = 0; i < contours.size(); i++) {
            // Only draw contours that look like potential markers (4 corners)
            std::vector<cv::Point> approx;
            double perimeter = cv::arcLength(contours[i], true);
            cv::approxPolyDP(contours[i], approx, 0.02 * perimeter, true);

            if (approx.size() == 4) {
                cv::drawContours(display_frame, contours, static_cast<int>(i), cv::Scalar(0, 255, 255), 2);
            }
        }

        // Add info overlay
        std::string info = "Frame: " + std::to_string(frame_count) +
                          " | Contours: " + std::to_string(contours.size());
        cv::putText(display_frame, info, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

        // Display frame
        cv::imshow("Minimal Debug Test", display_frame);

        // Handle window events
        int key = cv::waitKey(1);
        if (key == 27) { // ESC key
            std::cout << "🖥️ ESC pressed, exiting..." << std::endl;
            break;
        }

        // Progress update
        if (frame_count % 30 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
            double fps = (frame_count * 1000.0) / elapsed;
            std::cout << "📊 Frame: " << frame_count << ", FPS: " << std::fixed << std::setprecision(1) << fps
                      << ", Contours: " << contours.size() << std::endl;
        }
    }

    std::cout << "✅ Test completed successfully!" << std::endl;
    std::cout << "Total frames processed: " << frame_count << std::endl;

    cap.release();
    cv::destroyAllWindows();
    return 0;
}
