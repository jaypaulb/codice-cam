#pragma once

#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <functional>

namespace CodiceCam {

/**
 * @brief Manages webcam capture and provides frame data
 * 
 * The CameraManager handles webcam initialization, frame capture,
 * and provides a callback-based interface for frame processing.
 */
class CameraManager {
public:
    using FrameCallback = std::function<void(const cv::Mat&)>;
    
    /**
     * @brief Constructor
     * @param device_id Camera device ID (default: 0)
     * @param width Desired frame width (default: 640)
     * @param height Desired frame height (default: 480)
     */
    CameraManager(int device_id = 0, int width = 640, int height = 480);
    
    /**
     * @brief Destructor
     */
    ~CameraManager();
    
    /**
     * @brief Initialize the camera
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Start capturing frames
     * @param callback Function to call for each frame
     * @return true if successful, false otherwise
     */
    bool startCapture(FrameCallback callback);
    
    /**
     * @brief Stop capturing frames
     */
    void stopCapture();
    
    /**
     * @brief Check if camera is capturing
     * @return true if capturing, false otherwise
     */
    bool isCapturing() const;
    
    /**
     * @brief Get current frame dimensions
     * @return cv::Size with width and height
     */
    cv::Size getFrameSize() const;
    
    /**
     * @brief Set frame dimensions
     * @param width New width
     * @param height New height
     * @return true if successful, false otherwise
     */
    bool setFrameSize(int width, int height);
    
    /**
     * @brief Get camera device ID
     * @return Device ID
     */
    int getDeviceId() const;
    
    /**
     * @brief Check if camera is available
     * @return true if available, false otherwise
     */
    bool isAvailable() const;

private:
    int device_id_;
    int width_;
    int height_;
    std::unique_ptr<cv::VideoCapture> cap_;
    FrameCallback frame_callback_;
    bool capturing_;
    bool initialized_;
    
    /**
     * @brief Capture loop running in separate thread
     */
    void captureLoop();
    
    /**
     * @brief Validate frame dimensions
     * @param width Width to validate
     * @param height Height to validate
     * @return true if valid, false otherwise
     */
    bool validateDimensions(int width, int height) const;
};

} // namespace CodiceCam
