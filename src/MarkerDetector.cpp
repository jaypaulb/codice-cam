#include "MarkerDetector.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <bitset>

namespace CodiceCam {

// Debug output macro
#define DEBUG_OUT(x) if (debug_mode_) { std::cout << x; }
#define VERBOSE_OUT(x) if (verbose_mode_ || debug_mode_) { std::cout << x; }

MarkerDetector::MarkerDetector()
    : image_processor_(std::make_unique<ImageProcessor>())
    , min_marker_size_(40)
    , max_marker_size_(200)
    , min_confidence_(0.7)
    , debug_mode_(false)
    , verbose_mode_(false)
    , total_frames_processed_(0)
    , total_markers_detected_(0)
    , total_detection_attempts_(0)
{
    // Configure image processor for marker detection
    image_processor_->setPreprocessingParams(1, 1.3, 20);  // NO blur (kernel=1), enhanced contrast
    image_processor_->setEdgeDetectionParams(30, 100);     // Lower thresholds for better detection
    image_processor_->setContourFilterParams(500, 100000, 80);  // Larger area range for markers
}

MarkerDetector::~MarkerDetector() {
}

bool MarkerDetector::detectMarkers(const cv::Mat& frame, std::vector<CodiceMarker>& markers) {
    VERBOSE_OUT("🔍 [DEBUG] detectMarkers called with frame size: " << frame.cols << "x" << frame.rows << std::endl);

    if (frame.empty()) {
        std::cerr << "❌ Input frame is empty" << std::endl;
        return false;
    }

    total_frames_processed_++;
    markers.clear();
    VERBOSE_OUT("🔍 [DEBUG] Starting marker detection process..." << std::endl);

    try {
        // Step 1: Process frame for contour detection
        VERBOSE_OUT("🔍 [DEBUG] Step 1: Processing frame..." << std::endl);
        cv::Mat processed_frame;
        if (!image_processor_->processFrame(frame, processed_frame)) {
            std::cerr << "❌ Failed to process frame" << std::endl;
            return false;
        }
        VERBOSE_OUT("🔍 [DEBUG] Frame processed successfully, size: " << processed_frame.cols << "x" << processed_frame.rows << std::endl);

        // Get the preprocessed frame (for pattern reading)
        const cv::Mat& preprocessed_frame = image_processor_->getPreprocessedFrame();
        VERBOSE_OUT("🔍 [DEBUG] Preprocessed frame available, size: " << preprocessed_frame.cols << "x" << preprocessed_frame.rows << std::endl);

        // Step 2: Find potential marker contours
        VERBOSE_OUT("🔍 [DEBUG] Step 2: Finding contours..." << std::endl);
        std::vector<std::vector<cv::Point>> contours;
        if (!image_processor_->findMarkerContours(processed_frame, contours)) {
            VERBOSE_OUT("🔍 [DEBUG] No contours found - this is normal" << std::endl);
            return true;
        }
        VERBOSE_OUT("🔍 [DEBUG] Found " << contours.size() << " contours, starting processing..." << std::endl);

        // Step 3: Process each contour to detect markers
        VERBOSE_OUT("🔍 [DEBUG] Step 3: Processing " << contours.size() << " contours..." << std::endl);
        for (size_t i = 0; i < contours.size(); i++) {
            VERBOSE_OUT("🔍 [DEBUG] Processing contour " << (i+1) << "/" << contours.size() << " with " << contours[i].size() << " points" << std::endl);
            try {
                total_detection_attempts_++;

                if (debug_mode_) {
                    std::cout << "🔍 Processing contour " << (i+1) << "/" << contours.size() << std::endl;
                }

                DEBUG_OUT("🔍 [DEBUG] Calling processContour..." << std::endl);
                CodiceMarker marker;
                if (processContour(contours[i], marker)) {
                    DEBUG_OUT("🔍 [DEBUG] processContour returned true, confidence: " << marker.confidence << std::endl);
                    if (debug_mode_) {
                        std::cout << "✅ Marker detected with confidence: " << marker.confidence << std::endl;
                    }
                    if (marker.confidence >= min_confidence_) {
                        markers.push_back(marker);
                        total_markers_detected_++;
                        DEBUG_OUT("🔍 [DEBUG] Marker added to results" << std::endl);
                    }
                } else {
                    DEBUG_OUT("🔍 [DEBUG] processContour returned false" << std::endl);
                    if (debug_mode_) {
                        std::cout << "❌ Contour " << (i+1) << " did not match marker pattern" << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "❌ Error processing contour " << (i+1) << ": " << e.what() << std::endl;
                continue;
            }
        }
        VERBOSE_OUT("🔍 [DEBUG] Contour processing completed" << std::endl);

        // Step 4: Save debug information to files (avoiding OpenCV window issues)
        if (debug_mode_) {
            std::cout << "🔍 [DEBUG] Step 4: Saving debug information to files..." << std::endl;
            try {
                cv::Mat debug_frame = frame.clone();
                std::cout << "🔍 [DEBUG] Debug frame cloned, size: " << debug_frame.cols << "x" << debug_frame.rows << std::endl;

                drawDebugInfo(debug_frame, markers);
                std::cout << "🔍 [DEBUG] Debug info drawn" << std::endl;

                // Save debug frame to file instead of showing window
                std::cout << "🔍 [DEBUG] Saving debug frame to file..." << std::endl;
                cv::imwrite("debug_frame.jpg", debug_frame);
                std::cout << "🔍 [DEBUG] Debug frame saved to debug_frame.jpg" << std::endl;

                // Save processed frame to file
                std::cout << "🔍 [DEBUG] Saving processed frame to file..." << std::endl;
                cv::imwrite("processed_frame.jpg", processed_frame);
                std::cout << "🔍 [DEBUG] Processed frame saved to processed_frame.jpg" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "❌ Error in debug visualization: " << e.what() << std::endl;
            }
        }
        VERBOSE_OUT("🔍 [DEBUG] detectMarkers completed successfully" << std::endl);

        if (!markers.empty()) {
            VERBOSE_OUT("🎯 Detected " << markers.size() << " Codice markers" << std::endl);
        }

        return true;

    } catch (const cv::Exception& e) {
        std::cerr << "❌ OpenCV error in detectMarkers: " << e.what() << std::endl;
        return false;
    }
}

bool MarkerDetector::processContour(const std::vector<cv::Point>& contour, CodiceMarker& marker) {
    DEBUG_OUT("🔍 [DEBUG] processContour called with " << contour.size() << " points" << std::endl);
    try {
        // Approximate contour to get corner points
        DEBUG_OUT("🔍 [DEBUG] Approximating contour..." << std::endl);
        std::vector<cv::Point> approx;
        double epsilon = 0.02 * cv::arcLength(contour, true);
        cv::approxPolyDP(contour, approx, epsilon, true);
        DEBUG_OUT("🔍 [DEBUG] Contour approximated to " << approx.size() << " points" << std::endl);

    // Need exactly 4 corners for a Codice marker
    if (approx.size() != 4) {
        return false;
    }

    // Convert to Point2f for better precision
    std::vector<cv::Point2f> corners;
    for (const auto& point : approx) {
        corners.emplace_back(static_cast<float>(point.x), static_cast<float>(point.y));
    }

    // Sort corners properly for Codice marker orientation
    // We need: top-left, top-right, bottom-right, bottom-left
    std::vector<cv::Point2f> ordered_corners = sortCornersForMarker(corners);

    DEBUG_OUT("🔍 [DEBUG] Corners sorted for marker orientation:" << std::endl);
    for (size_t i = 0; i < ordered_corners.size(); i++) {
        DEBUG_OUT("🔍 [DEBUG] Ordered corner " << i << ": (" << ordered_corners[i].x << ", " << ordered_corners[i].y << ")" << std::endl);
    }

    // Calculate center point
    cv::Point2f center(0, 0);
    for (const auto& corner : ordered_corners) {
        center += corner;
    }
    center *= 0.25f;

    // Calculate rotation angle
    cv::Point2f top_left = ordered_corners[0];
    cv::Point2f top_right = ordered_corners[1];
    float angle = std::atan2(top_right.y - top_left.y, top_right.x - top_left.x) * 180.0f / CV_PI;

    // Check marker size
    float marker_size = cv::norm(ordered_corners[1] - ordered_corners[0]);
    if (marker_size < min_marker_size_ || marker_size > max_marker_size_) {
        return false;
    }

    // Extract and deskew marker region for pattern decoding
    DEBUG_OUT("🔍 [DEBUG] Extracting and deskewing marker region..." << std::endl);
    cv::Mat marker_region;
    const cv::Mat& preprocessed_frame = image_processor_->getPreprocessedFrame();
    if (!extractAndDeskewMarker(preprocessed_frame, ordered_corners, marker_region)) {
        DEBUG_OUT("🔍 [DEBUG] Failed to extract and deskew marker region" << std::endl);
        return false;
    }
    DEBUG_OUT("🔍 [DEBUG] Marker region extracted and deskewed, size: " << marker_region.cols << "x" << marker_region.rows << std::endl);

    // Decode the marker pattern
    DEBUG_OUT("🔍 [DEBUG] Decoding marker pattern..." << std::endl);
    int marker_id;
    double confidence;
    if (!decodeMarker(marker_region, marker_id, confidence)) {
        DEBUG_OUT("🔍 [DEBUG] Failed to decode marker pattern" << std::endl);
        return false;
    }
    DEBUG_OUT("🔍 [DEBUG] Marker decoded: ID=" << marker_id << ", confidence=" << confidence << std::endl);

    // Set marker properties
    marker.id = marker_id;
    marker.confidence = confidence;
    marker.center = center;
    marker.angle = angle;
    marker.corners = ordered_corners;

    return true;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error in processContour: " << e.what() << std::endl;
        return false;
    }
}

std::vector<cv::Point2f> MarkerDetector::sortCornersForMarker(const std::vector<cv::Point2f>& corners) {
        DEBUG_OUT("🔍 [DEBUG] Sorting corners for marker orientation..." << std::endl);

    if (corners.size() != 4) {
        DEBUG_OUT("🔍 [DEBUG] Invalid number of corners for sorting: " << corners.size() << std::endl);
        return corners;
    }

    // Debug: Print original corners
    DEBUG_OUT("🔍 [DEBUG] Original corners:" << std::endl);
    for (size_t i = 0; i < corners.size(); i++) {
        DEBUG_OUT("🔍 [DEBUG] Corner " << i << ": (" << corners[i].x << ", " << corners[i].y << ")" << std::endl);
    }

    // Find the center point
    cv::Point2f center(0, 0);
    for (const auto& corner : corners) {
        center += corner;
    }
    center *= 0.25f;
    DEBUG_OUT("🔍 [DEBUG] Center point: (" << center.x << ", " << center.y << ")" << std::endl);

    // Sort corners by their position relative to center
    std::vector<cv::Point2f> sorted_corners(4);

    // Find top-left (smallest x+y)
    // Find top-right (largest x-y)
    // Find bottom-right (largest x+y)
    // Find bottom-left (smallest x-y)

    for (const auto& corner : corners) {
        float x = corner.x - center.x;
        float y = corner.y - center.y;

        if (x <= 0 && y <= 0) {
            // Top-left quadrant
            sorted_corners[0] = corner;
        } else if (x >= 0 && y <= 0) {
            // Top-right quadrant
            sorted_corners[1] = corner;
        } else if (x >= 0 && y >= 0) {
            // Bottom-right quadrant
            sorted_corners[2] = corner;
        } else {
            // Bottom-left quadrant
            sorted_corners[3] = corner;
        }
    }

    DEBUG_OUT("🔍 [DEBUG] Sorted corners:" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] TL: (" << sorted_corners[0].x << ", " << sorted_corners[0].y << ")" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] TR: (" << sorted_corners[1].x << ", " << sorted_corners[1].y << ")" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] BR: (" << sorted_corners[2].x << ", " << sorted_corners[2].y << ")" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] BL: (" << sorted_corners[3].x << ", " << sorted_corners[3].y << ")" << std::endl);
    return sorted_corners;
}

bool MarkerDetector::extractAndDeskewMarker(const cv::Mat& frame, const std::vector<cv::Point2f>& corners, cv::Mat& marker_region) {
    DEBUG_OUT("🔍 [DEBUG] extractAndDeskewMarker called with " << corners.size() << " corners" << std::endl);

    // Validate input corners
    if (corners.size() != 4) {
        DEBUG_OUT("🔍 [DEBUG] Invalid number of corners: " << corners.size() << std::endl);
        return false;
    }

    // Print corner coordinates for debugging
    for (size_t i = 0; i < corners.size(); i++) {
        DEBUG_OUT("🔍 [DEBUG] Corner " << i << ": (" << corners[i].x << ", " << corners[i].y << ")" << std::endl);
    }

    // Define destination points for perfect square (5x5 grid = 100x100 pixels)
    // This creates a perfect square for Codice marker processing
    std::vector<cv::Point2f> dst_points = {
        cv::Point2f(0, 0),           // top-left
        cv::Point2f(99, 0),          // top-right
        cv::Point2f(99, 99),         // bottom-right
        cv::Point2f(0, 99)           // bottom-left
    };

    DEBUG_OUT("🔍 [DEBUG] Destination points defined for 100x100 square" << std::endl);

    // Get perspective transform matrix
    cv::Mat transform_matrix = cv::getPerspectiveTransform(corners, dst_points);
    DEBUG_OUT("🔍 [DEBUG] Perspective transform matrix calculated" << std::endl);

    // Use the PREPROCESSED frame (not the edge-detected frame) for marker extraction
    // This contains the actual marker content (white/black pattern) not just edges
    const cv::Mat& preprocessed_frame = image_processor_->getPreprocessedFrame();
    DEBUG_OUT("🔍 [DEBUG] Using preprocessed frame for marker extraction, size: " << preprocessed_frame.cols << "x" << preprocessed_frame.rows << std::endl);

    // Apply perspective transform to extract and deskew the marker
    cv::warpPerspective(preprocessed_frame, marker_region, transform_matrix, cv::Size(100, 100));
    DEBUG_OUT("🔍 [DEBUG] Perspective transform applied, result size: " << marker_region.cols << "x" << marker_region.rows << std::endl);

    // Validate the result
    if (marker_region.empty()) {
        DEBUG_OUT("🔍 [DEBUG] Resulting marker region is empty" << std::endl);
        return false;
    }

    // Create debug visualization with red grid overlay
    cv::Mat debug_marker = marker_region.clone();
    if (debug_marker.channels() == 1) {
        cv::cvtColor(debug_marker, debug_marker, cv::COLOR_GRAY2BGR);
    }

    // Draw 5x5 grid (100x100 image = 20x20 pixel cells)
    for (int i = 0; i <= 5; i++) {
        int pos = i * 20;
        // Vertical lines
        cv::line(debug_marker, cv::Point(pos, 0), cv::Point(pos, 100), cv::Scalar(0, 0, 255), 1);
        // Horizontal lines
        cv::line(debug_marker, cv::Point(0, pos), cv::Point(100, pos), cv::Scalar(0, 0, 255), 1);
    }

    // Highlight the inner 4x4 grid (cells 1,1 to 4,4) in green
    cv::rectangle(debug_marker, cv::Point(20, 20), cv::Point(80, 80), cv::Scalar(0, 255, 0), 2);

    // Save debug image with grid
    cv::imwrite("deskewed_marker_with_grid.jpg", debug_marker);
    DEBUG_OUT("🔍 [DEBUG] Saved deskewed marker with red grid overlay to deskewed_marker_with_grid.jpg" << std::endl);

    DEBUG_OUT("🔍 [DEBUG] Marker region successfully extracted and deskewed" << std::endl);
    return true;
}

bool MarkerDetector::extractMarkerRegion(const cv::Mat& frame, const std::vector<cv::Point2f>& corners, cv::Mat& marker_region) {
    // Legacy method - redirect to new method
    return extractAndDeskewMarker(frame, corners, marker_region);
}

bool MarkerDetector::decodeMarker(const cv::Mat& marker_region, int& marker_id, double& confidence) {
    if (debug_mode_) {
        DEBUG_OUT("🔍 [DEBUG] decodeMarker called with region size: " << marker_region.cols << "x" << marker_region.rows << std::endl);
    }

    // Convert to grayscale if needed
    cv::Mat gray_marker;
    if (marker_region.channels() == 3) {
        cv::cvtColor(marker_region, gray_marker, cv::COLOR_BGR2GRAY);
        DEBUG_OUT("🔍 [DEBUG] Converted from BGR to grayscale" << std::endl);
    } else {
        gray_marker = marker_region.clone();
        DEBUG_OUT("🔍 [DEBUG] Already grayscale, cloned" << std::endl);
    }

    // Apply threshold to get binary image
    // Use lower threshold since the "white" border is actually light gray (~70-90)
    cv::Mat binary_marker;
    cv::threshold(gray_marker, binary_marker, 70, 255, cv::THRESH_BINARY);

    // Debug: Check corner pattern to determine if inversion is needed
    // Sample the four corners to see the pattern
    uchar tl_pixel = binary_marker.at<uchar>(20, 20);  // TL corner region
    uchar tr_pixel = binary_marker.at<uchar>(20, 80);  // TR corner region
    uchar bl_pixel = binary_marker.at<uchar>(80, 20);  // BL corner region
    uchar br_pixel = binary_marker.at<uchar>(80, 80);  // BR corner region

    if (debug_mode_) {
        DEBUG_OUT("🔍 [DEBUG] Corner pixel values before inversion:" << std::endl);
        DEBUG_OUT("🔍 [DEBUG] TL: " << (int)tl_pixel << ", TR: " << (int)tr_pixel << ", BL: " << (int)bl_pixel << ", BR: " << (int)br_pixel << std::endl);
    }

    // Count white corners
    int white_corners = 0;
    if (tl_pixel > 127) white_corners++;
    if (tr_pixel > 127) white_corners++;
    if (bl_pixel > 127) white_corners++;
    if (br_pixel > 127) white_corners++;

        DEBUG_OUT("🔍 [DEBUG] White corners found: " << white_corners << std::endl);

    // Only invert if we have 0 or 4 white corners (should have exactly 1)
    if (white_corners == 0 || white_corners == 4) {
        DEBUG_OUT("🔍 [DEBUG] Invalid corner pattern (" << white_corners << " white corners), inverting image..." << std::endl);
        cv::bitwise_not(binary_marker, binary_marker);
    } else if (white_corners == 1) {
        DEBUG_OUT("🔍 [DEBUG] Valid corner pattern (1 white corner), no inversion needed" << std::endl);
    } else {
        DEBUG_OUT("🔍 [DEBUG] WARNING: Unexpected corner pattern (" << white_corners << " white corners)" << std::endl);
    }
    DEBUG_OUT("🔍 [DEBUG] Applied binary threshold, binary size: " << binary_marker.cols << "x" << binary_marker.rows << std::endl);

    // Debug: Check some border pixel values
    DEBUG_OUT("🔍 [DEBUG] Border pixel values:" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Top-left corner: gray=" << (int)gray_marker.at<uchar>(0, 0) << ", binary=" << (int)binary_marker.at<uchar>(0, 0) << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Top-right corner: gray=" << (int)gray_marker.at<uchar>(0, 99) << ", binary=" << (int)binary_marker.at<uchar>(0, 99) << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Bottom-left corner: gray=" << (int)gray_marker.at<uchar>(99, 0) << ", binary=" << (int)binary_marker.at<uchar>(99, 0) << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Bottom-right corner: gray=" << (int)gray_marker.at<uchar>(99, 99) << ", binary=" << (int)binary_marker.at<uchar>(99, 99) << std::endl);

    // Save debug images
    if (debug_mode_) {
        cv::imwrite("marker_region.jpg", marker_region);
        cv::imwrite("gray_marker.jpg", gray_marker);
        cv::imwrite("binary_marker.jpg", binary_marker);
        DEBUG_OUT("🔍 [DEBUG] Saved debug images: marker_region.jpg, gray_marker.jpg, binary_marker.jpg" << std::endl);
    }

    // Validate Codice marker pattern
    DEBUG_OUT("🔍 [DEBUG] Validating Codice pattern..." << std::endl);
    bool pattern_valid = isValidCodicePattern(binary_marker);
    DEBUG_OUT("🔍 [DEBUG] Pattern validation result: " << (pattern_valid ? "PASSED" : "FAILED") << std::endl);
    if (!pattern_valid) {
        DEBUG_OUT("🔍 [DEBUG] Pattern validation failed - this is not a valid Codice marker" << std::endl);
        return false; // Don't continue if pattern validation fails
    } else {
        DEBUG_OUT("🔍 [DEBUG] Pattern validation passed" << std::endl);
    }

    // Decode the binary pattern
    DEBUG_OUT("🔍 [DEBUG] Decoding binary pattern..." << std::endl);
    marker_id = decodeBinaryPattern(binary_marker);
    DEBUG_OUT("🔍 [DEBUG] Decoded marker ID: " << marker_id << std::endl);

    confidence = calculateConfidence(binary_marker, marker_id);
    DEBUG_OUT("🔍 [DEBUG] Calculated confidence: " << confidence << std::endl);

    bool valid_range = marker_id >= 0 && marker_id < 4096;
    DEBUG_OUT("🔍 [DEBUG] ID in valid range (0-4095): " << (valid_range ? "YES" : "NO") << std::endl);

    return valid_range;
}

bool MarkerDetector::validateMarkerPattern(const cv::Mat& binary_marker, int& marker_id, double& confidence) {
    return decodeMarker(binary_marker, marker_id, confidence);
}

cv::Mat MarkerDetector::getPerspectiveTransform(const cv::Mat& frame, const std::vector<cv::Point2f>& corners) {
    std::vector<cv::Point2f> dst_points = {
        cv::Point2f(0, 0), cv::Point2f(99, 0), cv::Point2f(99, 99), cv::Point2f(0, 99)
    };
    return cv::getPerspectiveTransform(corners, dst_points);
}

bool MarkerDetector::isValidCodicePattern(const cv::Mat& binary_marker) {
    DEBUG_OUT("🔍 [DEBUG] isValidCodicePattern called with size: " << binary_marker.cols << "x" << binary_marker.rows << std::endl);

    if (binary_marker.rows != 100 || binary_marker.cols != 100) {
        DEBUG_OUT("🔍 [DEBUG] Invalid size: expected 100x100, got " << binary_marker.cols << "x" << binary_marker.rows << std::endl);
        return false;
    }

    // Check outer border (should be white)
    DEBUG_OUT("🔍 [DEBUG] Checking outer border..." << std::endl);
    // Top border
    for (int x = 0; x < 100; x++) {
        if (binary_marker.at<uchar>(0, x) < 127) {
            DEBUG_OUT("🔍 [DEBUG] Top border failed at x=" << x << ", value=" << (int)binary_marker.at<uchar>(0, x) << std::endl);
            return false;
        }
        if (binary_marker.at<uchar>(99, x) < 127) {
            DEBUG_OUT("🔍 [DEBUG] Bottom border failed at x=" << x << ", value=" << (int)binary_marker.at<uchar>(99, x) << std::endl);
            return false;
        }
    }
    // Left and right borders
    for (int y = 0; y < 100; y++) {
        if (binary_marker.at<uchar>(y, 0) < 127) {
            DEBUG_OUT("🔍 [DEBUG] Left border failed at y=" << y << ", value=" << (int)binary_marker.at<uchar>(y, 0) << std::endl);
            return false;
        }
        if (binary_marker.at<uchar>(y, 99) < 127) {
            DEBUG_OUT("🔍 [DEBUG] Right border failed at y=" << y << ", value=" << (int)binary_marker.at<uchar>(y, 99) << std::endl);
            return false;
        }
    }
    DEBUG_OUT("🔍 [DEBUG] Outer border check passed" << std::endl);

    // Check corner markers (should be: tl=white, tr=black, bl=black, br=black)
    // Use smaller regions well inside the inner 4x4 grid to avoid border overlap
    DEBUG_OUT("🔍 [DEBUG] Checking corner markers..." << std::endl);

    // Top-left corner (10x10 region, well inside inner grid)
    cv::Rect tl_rect(20, 20, 10, 10);
    cv::Mat tl_region = binary_marker(tl_rect);
    int tl_white_pixels = cv::countNonZero(tl_region);
    double tl_ratio = (double)tl_white_pixels / tl_region.total();
    DEBUG_OUT("🔍 [DEBUG] TL corner: " << tl_white_pixels << "/" << tl_region.total() << " white pixels (" << (tl_ratio*100) << "%)" << std::endl);
    if (tl_white_pixels < tl_region.total() * 0.4) {
        DEBUG_OUT("🔍 [DEBUG] TL corner failed: expected >40% white, got " << (tl_ratio*100) << "%" << std::endl);
        return false;
    }

    // Top-right corner
    cv::Rect tr_rect(70, 20, 10, 10);
    cv::Mat tr_region = binary_marker(tr_rect);
    int tr_white_pixels = cv::countNonZero(tr_region);
    double tr_ratio = (double)tr_white_pixels / tr_region.total();
    DEBUG_OUT("🔍 [DEBUG] TR corner: " << tr_white_pixels << "/" << tr_region.total() << " white pixels (" << (tr_ratio*100) << "%)" << std::endl);
    if (tr_white_pixels > tr_region.total() * 0.6) {
        DEBUG_OUT("🔍 [DEBUG] TR corner failed: expected <60% white, got " << (tr_ratio*100) << "%" << std::endl);
        return false;
    }

    // Bottom-left corner
    cv::Rect bl_rect(20, 70, 10, 10);
    cv::Mat bl_region = binary_marker(bl_rect);
    int bl_white_pixels = cv::countNonZero(bl_region);
    double bl_ratio = (double)bl_white_pixels / bl_region.total();
    DEBUG_OUT("🔍 [DEBUG] BL corner: " << bl_white_pixels << "/" << bl_region.total() << " white pixels (" << (bl_ratio*100) << "%)" << std::endl);
    if (bl_white_pixels > bl_region.total() * 0.6) {
        DEBUG_OUT("🔍 [DEBUG] BL corner failed: expected <60% white, got " << (bl_ratio*100) << "%" << std::endl);
        return false;
    }

    // Bottom-right corner
    cv::Rect br_rect(70, 70, 10, 10);
    cv::Mat br_region = binary_marker(br_rect);
    int br_white_pixels = cv::countNonZero(br_region);
    double br_ratio = (double)br_white_pixels / br_region.total();
    DEBUG_OUT("🔍 [DEBUG] BR corner: " << br_white_pixels << "/" << br_region.total() << " white pixels (" << (br_ratio*100) << "%)" << std::endl);
    if (br_white_pixels > br_region.total() * 0.6) {
        DEBUG_OUT("🔍 [DEBUG] BR corner failed: expected <60% white, got " << (br_ratio*100) << "%" << std::endl);
        return false;
    }

    DEBUG_OUT("🔍 [DEBUG] All corner checks passed!" << std::endl);
    return true;
}

int MarkerDetector::decodeBinaryPattern(const cv::Mat& binary_marker) {
    DEBUG_OUT("🔍 [DEBUG] decodeBinaryPattern called with binary marker size: " << binary_marker.cols << "x" << binary_marker.rows << std::endl);

    // Step 1: Extract the 4x4 inner grid (excluding outer border)
    // In a 100x100 image with 5x5 grid (20x20 pixel cells), the inner 4x4 grid is from (20,20) to (80,80)
    cv::Rect inner_rect(20, 20, 60, 60);
    cv::Mat inner_region = binary_marker(inner_rect);
    DEBUG_OUT("🔍 [DEBUG] Extracted inner region: " << inner_region.cols << "x" << inner_region.rows << std::endl);

    // Step 2: Find the four corners and identify the white one
    DEBUG_OUT("🔍 [DEBUG] Analyzing corner pattern for orientation detection:" << std::endl);

    // Sample the four corners (center of each corner cell)
    struct CornerInfo {
        int row, col;
        int sample_x, sample_y;
        bool is_white;
        std::string name;
    };

    std::vector<CornerInfo> corners = {
        {0, 0, 7, 7, false, "TL"},   // Top-left
        {0, 3, 52, 7, false, "TR"},  // Top-right
        {3, 0, 7, 52, false, "BL"},  // Bottom-left
        {3, 3, 52, 52, false, "BR"}  // Bottom-right
    };

    // Read corner values
    for (auto& corner : corners) {
        uchar pixel_value = inner_region.at<uchar>(corner.sample_y, corner.sample_x);
        corner.is_white = pixel_value > 127;
        DEBUG_OUT("🔍 [DEBUG] " << corner.name << " corner [" << corner.row << "," << corner.col << "] at (" << corner.sample_x << "," << corner.sample_y << "): " << (corner.is_white ? "WHITE" : "BLACK") << " (value=" << (int)pixel_value << ")" << std::endl);
    }

    // Step 3: Determine rotation based on white corner position
    // Find which corner is white (should be only one)
    CornerInfo* white_corner = nullptr;
        for (auto& corner : corners) {
        if (corner.is_white) {
            if (white_corner != nullptr) {
                DEBUG_OUT("🔍 [DEBUG] ERROR: Multiple white corners found!" << std::endl);
                return -1;
            }
            white_corner = &corner;
        }
    }

    if (white_corner == nullptr) {
        DEBUG_OUT("🔍 [DEBUG] ERROR: No white corner found!" << std::endl);
        return -1;
    }

    DEBUG_OUT("🔍 [DEBUG] White corner found at: " << white_corner->name << " [" << white_corner->row << "," << white_corner->col << "]" << std::endl);

    // Step 4: Determine rotation and create orientation mapping
    // The white corner should be at TL (0,0) for correct orientation
    // If it's elsewhere, we need to rotate the reading order
    int rotation = 0; // 0=0°, 1=90°, 2=180°, 3=270°

    if (white_corner->name == "TR") {
        rotation = 1; // 90° clockwise
        DEBUG_OUT("🔍 [DEBUG] Marker rotated 90° clockwise - white corner at TR" << std::endl);
    } else if (white_corner->name == "BR") {
        rotation = 2; // 180°
        DEBUG_OUT("🔍 [DEBUG] Marker rotated 180° - white corner at BR" << std::endl);
    } else if (white_corner->name == "BL") {
        rotation = 3; // 270° clockwise (90° counter-clockwise)
        DEBUG_OUT("🔍 [DEBUG] Marker rotated 270° clockwise - white corner at BL" << std::endl);
    } else if (white_corner->name == "TL") {
        rotation = 0; // Correct orientation
        DEBUG_OUT("🔍 [DEBUG] Marker in correct orientation - white corner at TL" << std::endl);
    }

    // Step 5: Read the pattern with correct orientation
    DEBUG_OUT("🔍 [DEBUG] Reading pattern with rotation " << rotation << ":" << std::endl);

    int marker_id = 0;
    int bit_position = 0;

    // Create a 4x4 pattern array
    bool pattern[4][4];
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int sample_x = col * 15 + 7;
            int sample_y = row * 15 + 7;
            uchar pixel_value = inner_region.at<uchar>(sample_y, sample_x);
            pattern[row][col] = pixel_value > 127;
        }
    }

    // Apply rotation to the pattern
    bool rotated_pattern[4][4];
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int src_row, src_col;

            // Apply rotation transformation
            switch (rotation) {
                case 0: // No rotation
                    src_row = row;
                    src_col = col;
                    break;
                case 1: // 90° clockwise: (row,col) -> (col, 3-row)
                    src_row = col;
                    src_col = 3 - row;
                    break;
                case 2: // 180°: (row,col) -> (3-row, 3-col)
                    src_row = 3 - row;
                    src_col = 3 - col;
                    break;
                case 3: // 270° clockwise: (row,col) -> (3-col, row)
                    src_row = 3 - col;
                    src_col = row;
                    break;
            }

            rotated_pattern[row][col] = pattern[src_row][src_col];
        }
    }

        // Now read the rotated pattern as if it's in correct orientation
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            bool is_white = rotated_pattern[row][col];

            DEBUG_OUT("🔍 [DEBUG] Cell [" << row << "," << col << "]: " << (is_white ? "WHITE" : "BLACK"));

            // Set bit if white (skip corner bits as they're fixed)
            bool is_corner = (row == 0 && col == 0) ||  // TL corner (fixed to 1)
                           (row == 0 && col == 3) ||  // TR corner (fixed to 0)
                           (row == 3 && col == 0) ||  // BL corner (fixed to 0)
                           (row == 3 && col == 3);    // BR corner (fixed to 0)

            if (is_white && !is_corner) {
                marker_id |= (1 << bit_position);
                DEBUG_OUT(" -> bit " << bit_position << " set");
            }

            // Only increment bit position for non-corner bits
            if (!is_corner) {
                bit_position++;
            }

            // Debug: Show corner status
            if (is_corner) {
                DEBUG_OUT(" (corner - fixed, not counted)");
            }
            DEBUG_OUT(std::endl);
        }
    }

        DEBUG_OUT("🔍 [DEBUG] Decoded marker ID: " << marker_id << " (binary: " << std::bitset<11>(marker_id) << ")" << std::endl);

    // Debug: Print the visual pattern we're reading (after rotation)
    DEBUG_OUT("🔍 [DEBUG] Visual pattern after rotation (W=white, B=black):" << std::endl);
    for (int row = 0; row < 4; row++) {
        DEBUG_OUT("🔍 [DEBUG] Row " << row << ": ");
        for (int col = 0; col < 4; col++) {
            DEBUG_OUT((rotated_pattern[row][col] ? "W" : "B") << " ");
        }
        DEBUG_OUT(std::endl);
    }

    return marker_id;
}

double MarkerDetector::calculateConfidence(const cv::Mat& binary_marker, int decoded_id) {
    // Simple confidence calculation based on pattern clarity
    // This could be enhanced with more sophisticated analysis

    double confidence = 0.5;  // Base confidence

    // Check if the decoded ID is in valid range
    if (decoded_id >= 0 && decoded_id < 4096) {
        confidence += 0.3;
    }

    // Check pattern consistency (could be enhanced)
    confidence += 0.2;

    return std::min(confidence, 1.0);
}

void MarkerDetector::drawDebugInfo(cv::Mat& frame, const std::vector<CodiceMarker>& markers) {
    for (const auto& marker : markers) {
        // Draw marker outline
        std::vector<cv::Point> int_corners;
        for (const auto& corner : marker.corners) {
            int_corners.emplace_back(static_cast<int>(corner.x), static_cast<int>(corner.y));
        }

        cv::polylines(frame, int_corners, true, cv::Scalar(0, 255, 0), 2);

        // Draw center point
        cv::circle(frame, cv::Point(static_cast<int>(marker.center.x), static_cast<int>(marker.center.y)),
                   5, cv::Scalar(0, 0, 255), -1);

        // Draw marker ID and confidence
        std::string info = "ID:" + std::to_string(marker.id) + " C:" + std::to_string(marker.confidence).substr(0, 3);
        cv::putText(frame, info, cv::Point(static_cast<int>(marker.center.x) - 20, static_cast<int>(marker.center.y) - 20),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    }
}

void MarkerDetector::setDetectionParams(int min_marker_size, int max_marker_size, double min_confidence) {
    min_marker_size_ = min_marker_size;
    max_marker_size_ = max_marker_size;
    min_confidence_ = min_confidence;

    std::cout << "⚙️ Detection params updated: size=[" << min_marker_size_
              << "," << max_marker_size_ << "], confidence=" << min_confidence_ << std::endl;
}

void MarkerDetector::setDebugMode(bool enable) {
    debug_mode_ = enable;
    if (enable) {
        // Only create window if we're not in a headless environment
        try {
            cv::namedWindow("Marker Detection Debug", cv::WINDOW_AUTOSIZE);
        } catch (const cv::Exception& e) {
            // Ignore window creation errors (e.g., in headless environments)
        }
    } else {
        try {
            cv::destroyWindow("Marker Detection Debug");
        } catch (const cv::Exception& e) {
            // Ignore window destruction errors
        }
    }
    VERBOSE_OUT("🐛 Debug mode " << (enable ? "enabled" : "disabled") << std::endl);
}

void MarkerDetector::setVerboseMode(bool enable) {
    verbose_mode_ = enable;
    VERBOSE_OUT("📝 Verbose mode " << (enable ? "enabled" : "disabled") << std::endl);
}

std::string MarkerDetector::getDetectionStats() const {
    std::string stats = "Marker Detection Statistics:\n";
    stats += "  Frames processed: " + std::to_string(total_frames_processed_) + "\n";
    stats += "  Detection attempts: " + std::to_string(total_detection_attempts_) + "\n";
    stats += "  Markers detected: " + std::to_string(total_markers_detected_) + "\n";
    if (total_frames_processed_ > 0) {
        double detection_rate = (double)total_markers_detected_ / total_frames_processed_;
        stats += "  Detection rate: " + std::to_string(detection_rate).substr(0, 4) + " markers/frame";
    }
    return stats;
}

bool MarkerDetector::testDecodeMarker(const cv::Mat& marker_region, int& marker_id, double& confidence) {
    std::cout << "🧪 [TEST] testDecodeMarker called with region size: " << marker_region.cols << "x" << marker_region.rows << std::endl;
    return decodeMarker(marker_region, marker_id, confidence);
}

} // namespace CodiceCam
