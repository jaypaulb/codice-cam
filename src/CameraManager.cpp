#include "CameraManager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

namespace CodiceCam {

CameraManager::CameraManager(int device_id, int width, int height)
    : device_id_(device_id)
    , width_(width)
    , height_(height)
    , cap_(nullptr)
    , capturing_(false)
    , initialized_(false)
{
}

CameraManager::~CameraManager() {
    stopCapture();
}

bool CameraManager::initialize() {
    if (initialized_) {
        return true;
    }

    // Validate dimensions
    if (!validateDimensions(width_, height_)) {
        std::cerr << "❌ Invalid frame dimensions: " << width_ << "x" << height_ << std::endl;
        return false;
    }

    // Create VideoCapture instance
    cap_ = std::make_unique<cv::VideoCapture>(device_id_);

    if (!cap_->isOpened()) {
        std::cerr << "❌ Error: Could not open camera device " << device_id_ << std::endl;
        return false;
    }

    // Set frame dimensions
    cap_->set(cv::CAP_PROP_FRAME_WIDTH, width_);
    cap_->set(cv::CAP_PROP_FRAME_HEIGHT, height_);
    
    // Set target FPS to 30
    cap_->set(cv::CAP_PROP_FPS, 30.0);

    // Verify actual dimensions and FPS
    int actual_width = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_WIDTH));
    int actual_height = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_HEIGHT));
    double actual_fps = cap_->get(cv::CAP_PROP_FPS);

    std::cout << "📹 Camera initialized: " << actual_width << "x" << actual_height 
              << " @ " << std::fixed << std::setprecision(1) << actual_fps << " FPS" << std::endl;

    // Update dimensions to actual values
    width_ = actual_width;
    height_ = actual_height;

    initialized_ = true;
    return true;
}

bool CameraManager::startCapture(FrameCallback callback) {
    if (!initialized_) {
        std::cerr << "❌ Camera not initialized. Call initialize() first." << std::endl;
        return false;
    }

    if (capturing_) {
        std::cerr << "⚠️ Camera already capturing." << std::endl;
        return false;
    }

    if (!callback) {
        std::cerr << "❌ Invalid callback function." << std::endl;
        return false;
    }

    frame_callback_ = callback;
    capturing_ = true;

    // Start capture loop in separate thread
    std::thread capture_thread(&CameraManager::captureLoop, this);
    capture_thread.detach();

    std::cout << "🎥 Camera capture started" << std::endl;
    return true;
}

void CameraManager::stopCapture() {
    if (!capturing_) {
        return;
    }

    capturing_ = false;

    // Wait a bit for the capture loop to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "🛑 Camera capture stopped" << std::endl;
}

bool CameraManager::isCapturing() const {
    return capturing_;
}

cv::Size CameraManager::getFrameSize() const {
    return cv::Size(width_, height_);
}

bool CameraManager::setFrameSize(int width, int height) {
    if (!validateDimensions(width, height)) {
        std::cerr << "❌ Invalid frame dimensions: " << width << "x" << height << std::endl;
        return false;
    }

    if (capturing_) {
        std::cerr << "❌ Cannot change frame size while capturing." << std::endl;
        return false;
    }

    if (initialized_ && cap_) {
        cap_->set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap_->set(cv::CAP_PROP_FRAME_HEIGHT, height);

        // Verify actual dimensions
        int actual_width = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_WIDTH));
        int actual_height = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_HEIGHT));

        width_ = actual_width;
        height_ = actual_height;

        std::cout << "📐 Frame size updated: " << width_ << "x" << height_ << std::endl;
    } else {
        width_ = width;
        height_ = height;
    }

    return true;
}

int CameraManager::getDeviceId() const {
    return device_id_;
}

bool CameraManager::isAvailable() const {
    if (!cap_) {
        return false;
    }
    return cap_->isOpened();
}

void CameraManager::captureLoop() {
    cv::Mat frame;
    auto last_frame_time = std::chrono::steady_clock::now();
    const auto target_frame_duration = std::chrono::milliseconds(33); // ~30 FPS (1000ms/30fps = 33.33ms)

    while (capturing_) {
        auto frame_start_time = std::chrono::steady_clock::now();
        
        if (!cap_->read(frame)) {
            std::cerr << "❌ Failed to read frame from camera" << std::endl;
            break;
        }

        if (frame.empty()) {
            std::cerr << "⚠️ Received empty frame" << std::endl;
            continue;
        }

        // Call the callback with the frame
        if (frame_callback_) {
            frame_callback_(frame);
        }

        // Calculate frame processing time
        auto frame_end_time = std::chrono::steady_clock::now();
        auto frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end_time - frame_start_time);
        
        // Sleep to maintain target FPS
        if (frame_duration < target_frame_duration) {
            auto sleep_duration = target_frame_duration - frame_duration;
            std::this_thread::sleep_for(sleep_duration);
        }
        
        last_frame_time = frame_end_time;
    }
}

bool CameraManager::validateDimensions(int width, int height) const {
    // Basic validation - reasonable limits
    return width > 0 && height > 0 &&
           width <= 4096 && height <= 4096 &&
           width >= 160 && height >= 120;
}

} // namespace CodiceCam
