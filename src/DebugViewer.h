#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include "MarkerDetector.h"  // For CodiceMarker struct definition

namespace CodiceCam {

/**
 * @brief Lightweight debug viewer for real-time camera feed with contour overlays
 *
 * This class provides a simple, reliable way to visualize:
 * - Live camera feed
 * - Basic edge detection
 * - Contour outlines (yellow for potential candidates)
 * - Detected marker information (green for valid markers)
 */
class DebugViewer {
public:
    /**
     * @brief Constructor
     */
    DebugViewer();

    /**
     * @brief Destructor
     */
    ~DebugViewer();

    /**
     * @brief Initialize the debug viewer window
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Process a frame and display debug information
     * @param frame Input camera frame
     * @param show_contours Whether to show contour overlays
     * @param show_edges Whether to show edge detection overlay
     * @return true if frame was processed successfully
     */
    bool processFrame(const cv::Mat& frame, bool show_contours = true, bool show_edges = false);

    /**
     * @brief Add marker information overlay
     * @param markers Vector of detected markers
     */
    void addMarkerOverlays(const std::vector<CodiceMarker>& markers);

    /**
     * @brief Check if the viewer window is still open
     * @return true if window is open, false if closed
     */
    bool isWindowOpen() const;

    /**
     * @brief Close the debug viewer window
     */
    void close();

private:
    bool window_initialized_;
    std::string window_name_;
    cv::Mat current_frame_;

    // Simple contour detection (much lighter than full marker detection)
    std::vector<std::vector<cv::Point>> findSimpleContours(const cv::Mat& frame);
    cv::Mat detectEdges(const cv::Mat& frame);

    // Drawing helpers
    void drawContourOverlays(cv::Mat& frame, const std::vector<std::vector<cv::Point>>& contours);
    void drawEdgeOverlay(cv::Mat& frame, const cv::Mat& edges);
    void drawInfoOverlay(cv::Mat& frame, int frame_count, int contour_count);
};

} // namespace CodiceCam
