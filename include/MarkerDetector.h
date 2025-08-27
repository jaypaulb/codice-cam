#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include "ImageProcessor.h"

namespace CodiceCam {

/**
 * @brief Represents a detected Codice marker
 */
struct CodiceMarker {
    int id;                    // Decoded marker ID (0-4095 for 4x4 markers)
    cv::Point2f center;        // Center point of the marker
    float angle;               // Rotation angle in degrees
    std::vector<cv::Point2f> corners;  // Four corner points
    double confidence;         // Detection confidence (0.0-1.0)

    CodiceMarker() : id(-1), angle(0.0), confidence(0.0) {}
};

/**
 * @brief Detects and decodes Codice markers in camera frames
 *
 * The MarkerDetector processes camera frames to find and decode
 * Codice markers according to the 4x4 format specification.
 */
class MarkerDetector {
public:
    /**
     * @brief Constructor
     */
    MarkerDetector();

    /**
     * @brief Destructor
     */
    ~MarkerDetector();

    /**
     * @brief Detect markers in a camera frame
     * @param frame Input camera frame
     * @param markers Output vector of detected markers
     * @return true if detection successful, false otherwise
     */
    bool detectMarkers(const cv::Mat& frame, std::vector<CodiceMarker>& markers);

    /**
     * @brief Set detection parameters
     * @param min_marker_size Minimum marker size in pixels
     * @param max_marker_size Maximum marker size in pixels
     * @param min_confidence Minimum detection confidence threshold
     */
    void setDetectionParams(int min_marker_size = 40, int max_marker_size = 200, double min_confidence = 0.7);

    /**
     * @brief Enable/disable debug visualization
     * @param enable true to enable debug output, false to disable
     */
    void setDebugMode(bool enable);

    /**
     * @brief Enable/disable verbose logging
     * @param enable true to enable verbose output, false to disable
     */
    void setVerboseMode(bool enable);

    /**
     * @brief Get detection statistics
     * @return String with detection statistics
     */
    std::string getDetectionStats() const;

    /**
     * @brief Test marker decoding with a pre-extracted marker region
     * @param marker_region Pre-extracted 100x100 marker region
     * @param marker_id Output marker ID
     * @param confidence Output confidence score
     * @return true if marker was successfully decoded
     */
    bool testDecodeMarker(const cv::Mat& marker_region, int& marker_id, double& confidence);

private:
    std::unique_ptr<ImageProcessor> image_processor_;

    // Detection parameters
    int min_marker_size_;
    int max_marker_size_;
    double min_confidence_;
    bool debug_mode_;
    bool verbose_mode_;
    bool quiet_mode_;  // Suppress most debug output

    // Detection statistics
    mutable int total_frames_processed_;
    mutable int total_markers_detected_;
    mutable int total_detection_attempts_;

    // Internal detection methods
    bool processContour(const std::vector<cv::Point>& contour, CodiceMarker& marker);
    std::vector<cv::Point2f> sortCornersForMarker(const std::vector<cv::Point2f>& corners);
    bool extractAndDeskewMarker(const cv::Mat& frame, const std::vector<cv::Point2f>& corners, cv::Mat& marker_region);
    bool extractMarkerRegion(const cv::Mat& frame, const std::vector<cv::Point2f>& corners, cv::Mat& marker_region);
    bool decodeMarker(const cv::Mat& marker_region, int& marker_id, double& confidence);
    bool validateMarkerPattern(const cv::Mat& binary_marker, int& marker_id, double& confidence);

    /**
     * @brief Perspective transform to get square marker view
     */
    cv::Mat getPerspectiveTransform(const cv::Mat& frame, const std::vector<cv::Point2f>& corners);

    /**
     * @brief Check if detected pattern matches Codice marker format
     */
    bool isValidCodicePattern(const cv::Mat& binary_marker);

    /**
     * @brief Decode binary pattern to marker ID
     */
    int decodeBinaryPattern(const cv::Mat& binary_marker);

    /**
     * @brief Calculate detection confidence based on pattern quality
     */
    double calculateConfidence(const cv::Mat& binary_marker, int decoded_id);

    /**
     * @brief Draw debug visualization
     */
    void drawDebugInfo(cv::Mat& frame, const std::vector<CodiceMarker>& markers);
};

} // namespace CodiceCam
