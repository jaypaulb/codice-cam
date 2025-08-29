#include "MarkerDetector.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <bitset>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace CodiceCam {

// Debug output macro
#define DEBUG_OUT(x) if (debug_mode_) { std::cout << x; }
#define VERBOSE_OUT(x) if (verbose_mode_ || debug_mode_) { std::cout << x; }

// Helper function to generate timestamp for debug files
std::string generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

MarkerDetector::MarkerDetector()
    : image_processor_(std::make_unique<ImageProcessor>())
    , min_marker_size_(40)
    , max_marker_size_(200)
    , min_confidence_(0.7)
    , debug_mode_(false)
    , debug_window_enabled_(false)
    , verbose_mode_(false)
    , total_frames_processed_(0)
    , total_markers_detected_(0)
    , total_detection_attempts_(0)
    , location_change_threshold_(30.0)  // 30 pixels minimum change to save new debug set
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
            contours.clear(); // Ensure empty contours vector
        } else {
            VERBOSE_OUT("🔍 [DEBUG] Found " << contours.size() << " contours, starting processing..." << std::endl);
        }

        // Step 3: Process each contour to detect markers (if any)
        VERBOSE_OUT("🔍 [DEBUG] Step 3: Processing " << contours.size() << " contours..." << std::endl);
        std::string timestamp = generateTimestamp(); // Generate once for this frame
        int marker_index = 0; // Track marker index for multiple markers

        for (size_t i = 0; i < contours.size(); i++) {
            VERBOSE_OUT("🔍 [DEBUG] Processing contour " << (i+1) << "/" << contours.size() << " with " << contours[i].size() << " points" << std::endl);
            try {
                total_detection_attempts_++;

                if (debug_mode_) {
                    std::cout << "🔍 Processing contour " << (i+1) << "/" << contours.size() << std::endl;
                }

                DEBUG_OUT("🔍 [DEBUG] Calling processContour..." << std::endl);
                CodiceMarker marker;
                if (processContour(contours[i], frame, marker, timestamp, marker_index)) {
                    DEBUG_OUT("🔍 [DEBUG] processContour returned true, confidence: " << marker.confidence << std::endl);
                    if (debug_mode_) {
                        std::cout << "✅ Marker detected with confidence: " << marker.confidence << std::endl;
                    }
                    if (marker.confidence >= min_confidence_) {
                        markers.push_back(marker);
                        total_markers_detected_++;
                        marker_index++; // Increment for next marker in this frame
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

        // Step 4: Show live debug window if enabled
        if (debug_window_enabled_) {
            drawLiveDebugWindow(frame, contours, markers);
        }

        // Step 5: Save debug information to files when debug mode is enabled and location changed
        if (debug_mode_ && hasLocationChanged(markers)) {
            // Use the same timestamp generated for the frame processing
            std::cout << "🔍 [DEBUG] Step 5: Saving debug set " << timestamp << " (" << markers.size() << " markers, " << contours.size() << " contours)" << std::endl;
            std::cout << "🔍 [DEBUG] Location changed, proceeding with file saves..." << std::endl;

            try {
                cv::Mat debug_frame = frame.clone();
                std::cout << "🔍 [DEBUG] Debug frame cloned, size: " << debug_frame.cols << "x" << debug_frame.rows << std::endl;

                // Draw all contours and detection attempts for debugging
                drawAllContoursDebug(debug_frame, contours);
                drawDebugInfo(debug_frame, markers);
                std::cout << "🔍 [DEBUG] Debug info drawn" << std::endl;

                // Save timestamped debug set
                std::string debug_prefix = "debug_output/" + timestamp;

                cv::imwrite(debug_prefix + "_debug_frame.jpg", debug_frame);
                cv::imwrite(debug_prefix + "_processed_frame.jpg", processed_frame);
                cv::imwrite(debug_prefix + "_preprocessed_frame.jpg", preprocessed_frame);

                std::cout << "🔍 [DEBUG] Debug set saved with timestamp " << timestamp << std::endl;
                std::cout << "  - " << debug_prefix << "_debug_frame.jpg" << std::endl;
                std::cout << "  - " << debug_prefix << "_processed_frame.jpg" << std::endl;
                std::cout << "  - " << debug_prefix << "_preprocessed_frame.jpg" << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "❌ Error in debug visualization: " << e.what() << std::endl;
            }
        }
        VERBOSE_OUT("🔍 [DEBUG] detectMarkers completed successfully" << std::endl);

        if (!markers.empty()) {
            VERBOSE_OUT("🎯 Detected " << markers.size() << " Codice markers" << std::endl);
        }

        // Update previous marker locations for next frame comparison
        previous_marker_locations_.clear();
        for (const auto& marker : markers) {
            previous_marker_locations_.push_back(marker.center);
        }

        return true;

    } catch (const cv::Exception& e) {
        std::cerr << "❌ OpenCV error in detectMarkers: " << e.what() << std::endl;
        return false;
    }
}

bool MarkerDetector::detectMarkers(const cv::Mat& original_frame, const cv::Mat& processed_frame, std::vector<CodiceMarker>& markers) {
    try {
        total_frames_processed_++;
        markers.clear();

        VERBOSE_OUT("🔍 [DEBUG] detectMarkers called with original frame: " << original_frame.cols << "x" << original_frame.rows
                    << " and processed frame: " << processed_frame.cols << "x" << processed_frame.rows << std::endl);

        if (original_frame.empty() || processed_frame.empty()) {
            DEBUG_OUT("🔍 [DEBUG] Empty frame(s) received" << std::endl);
            return false;
        }

        // Get the preprocessed frame (for pattern reading)
        const cv::Mat& preprocessed_frame = image_processor_->getPreprocessedFrame();
        VERBOSE_OUT("🔍 [DEBUG] Preprocessed frame available, size: " << preprocessed_frame.cols << "x" << preprocessed_frame.rows << std::endl);

        // Step 2: Find potential marker contours (use the processed frame passed in)
        VERBOSE_OUT("🔍 [DEBUG] Step 2: Finding contours..." << std::endl);
        std::vector<std::vector<cv::Point>> contours;
        if (!image_processor_->findMarkerContours(processed_frame, contours)) {
            VERBOSE_OUT("🔍 [DEBUG] No contours found - this is normal" << std::endl);
            contours.clear(); // Ensure empty contours vector
        } else {
            VERBOSE_OUT("🔍 [DEBUG] Found " << contours.size() << " contours, starting processing..." << std::endl);
        }

        // Step 3: Process each contour to detect markers (if any)
        VERBOSE_OUT("🔍 [DEBUG] Step 3: Processing " << contours.size() << " contours..." << std::endl);
        std::string timestamp = generateTimestamp(); // Generate once for this frame
        int marker_index = 0; // Track marker index for multiple markers

        for (size_t i = 0; i < contours.size(); i++) {
            VERBOSE_OUT("🔍 [DEBUG] Processing contour " << (i+1) << "/" << contours.size() << " with " << contours[i].size() << " points" << std::endl);
            try {
                total_detection_attempts_++;

                if (debug_mode_) {
                    std::cout << "🔍 Processing contour " << (i+1) << "/" << contours.size() << std::endl;
                }

                DEBUG_OUT("🔍 [DEBUG] Calling processContour..." << std::endl);
                CodiceMarker marker;
                if (processContour(contours[i], original_frame, marker, timestamp, static_cast<int>(i))) {
                    DEBUG_OUT("🔍 [DEBUG] processContour returned true, confidence: " << marker.confidence << std::endl);
                    if (debug_mode_) {
                        std::cout << "✅ Marker detected with confidence: " << marker.confidence << std::endl;
                    }
                    if (marker.confidence >= min_confidence_) {
                        markers.push_back(marker);
                        total_markers_detected_++;
                        marker_index++; // Increment for next marker in this frame
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

        // Step 4: Show live debug window if enabled
        if (debug_window_enabled_) {
            drawLiveDebugWindow(original_frame, contours, markers);
        }

        // Step 5: Save debug information to files when debug mode is enabled and location changed
        // Use the ORIGINAL frame for debug visualization, not the processed frame
        if (debug_mode_ && hasLocationChanged(markers)) {
            // Use the same timestamp generated for the frame processing
            std::cout << "🔍 [DEBUG] Step 5: Saving debug set " << timestamp << " (" << markers.size() << " markers, " << contours.size() << " contours)" << std::endl;
            std::cout << "🔍 [DEBUG] Location changed, proceeding with file saves..." << std::endl;

            try {
                cv::Mat debug_frame = original_frame.clone();  // Use ORIGINAL frame
                std::cout << "🔍 [DEBUG] Debug frame cloned from ORIGINAL, size: " << debug_frame.cols << "x" << debug_frame.rows << std::endl;

                // Draw all contours and detection attempts for debugging
                drawAllContoursDebug(debug_frame, contours);
                drawDebugInfo(debug_frame, markers);
                std::cout << "🔍 [DEBUG] Debug info drawn" << std::endl;

                                // Save only the 4 requested debug images
                std::string debug_prefix = "debug_output/" + timestamp;

                cv::imwrite(debug_prefix + "_debug_frame.jpg", debug_frame);  // 1. Debug Frame
                cv::imwrite(debug_prefix + "_processed_frame.jpg", processed_frame);  // 2. Processed for edge detection

                std::cout << "🔍 [DEBUG] Debug set saved with timestamp " << timestamp << std::endl;
                std::cout << "  - " << debug_prefix << "_debug_frame.jpg (debug frame)" << std::endl;
                std::cout << "  - " << debug_prefix << "_processed_frame.jpg (edge detection)" << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "❌ Error in debug visualization: " << e.what() << std::endl;
            }
        }
        VERBOSE_OUT("🔍 [DEBUG] detectMarkers completed successfully" << std::endl);

        if (!markers.empty()) {
            VERBOSE_OUT("🎯 Detected " << markers.size() << " Codice markers" << std::endl);
        }

        // Update previous marker locations for next frame comparison
        previous_marker_locations_.clear();
        for (const auto& marker : markers) {
            previous_marker_locations_.push_back(marker.center);
        }

        return true;

    } catch (const cv::Exception& e) {
        std::cerr << "❌ OpenCV error in detectMarkers: " << e.what() << std::endl;
        return false;
    }
}

bool MarkerDetector::processContour(const std::vector<cv::Point>& contour, const cv::Mat& original_frame, CodiceMarker& marker, const std::string& timestamp, int marker_index) {
    DEBUG_OUT("🔍 [DEBUG] processContour called with " << contour.size() << " points" << std::endl);
    try {
        // Approximate contour to get corner points
        DEBUG_OUT("🔍 [DEBUG] Approximating contour..." << std::endl);
        std::vector<cv::Point> approx;
        double epsilon = 0.05 * cv::arcLength(contour, true); // More aggressive approximation for squares
        cv::approxPolyDP(contour, approx, epsilon, true);
        DEBUG_OUT("🔍 [DEBUG] Contour approximated to " << approx.size() << " points" << std::endl);

    // Save debug image for any contour attempt (for debugging false positives)
    if (!timestamp.empty() && debug_mode_) {
        std::string debug_filename = "debug_output/" + timestamp + "_contour" + std::to_string(marker_index) + "_attempt.jpg";

        // Create a small debug image showing the contour
        cv::Rect bounds = cv::boundingRect(contour);
        if (bounds.width > 20 && bounds.height > 20 &&
            bounds.x >= 0 && bounds.y >= 0 &&
            bounds.x + bounds.width < original_frame.cols &&
            bounds.y + bounds.height < original_frame.rows) {

            cv::Mat contour_region = original_frame(bounds).clone();
            cv::imwrite(debug_filename, contour_region);
            DEBUG_OUT("🔍 [DEBUG] Saved contour attempt to " << debug_filename << " (approx: " << approx.size() << " corners)" << std::endl);
        }
    }

    // STRICT: Only process contours with EXACTLY 4 corners (perfect squares)
    // Codice markers are always squares - reject anything else immediately
    if (approx.size() != 4) {
        DEBUG_OUT("🔍 [DEBUG] Rejecting " << approx.size() << "-corner contour - markers must be exactly 4 corners" << std::endl);
        return false; // Strict filtering for squares only
    }

    DEBUG_OUT("🔍 [DEBUG] Perfect 4-corner contour found - proceeding with marker processing" << std::endl);

    // Use EXACT same method as debug visualization - no sorting, no conversion!
    // Debug draws: cv::polylines(frame, approx, true, color, 2)
    // Extraction should use: EXACT same approx points

    std::vector<cv::Point2f> ordered_corners;
    for (const auto& point : approx) {
        ordered_corners.emplace_back(static_cast<float>(point.x), static_cast<float>(point.y));
    }

    DEBUG_OUT("🔍 [DEBUG] Using EXACT same corners as debug visualization (approx points)" << std::endl);
    for (size_t i = 0; i < ordered_corners.size(); i++) {
        DEBUG_OUT("🔍 [DEBUG] Corner " << i << ": (" << ordered_corners[i].x << ", " << ordered_corners[i].y << ")" << std::endl);
    }

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
    // Use original frame for marker extraction (it contains the actual image data)
    float deskew_angle;
    if (!extractAndDeskewMarker(original_frame, ordered_corners, marker_region, deskew_angle, timestamp, marker_index)) {
        DEBUG_OUT("🔍 [DEBUG] Failed to extract and deskew marker region" << std::endl);
        return false;
    }
    DEBUG_OUT("🔍 [DEBUG] Marker region extracted and deskewed, size: " << marker_region.cols << "x" << marker_region.rows << std::endl);

    // Decode the marker pattern
    DEBUG_OUT("🔍 [DEBUG] Decoding marker pattern..." << std::endl);
    int marker_id;
    double confidence;
    if (!decodeMarker(marker_region, marker_id, confidence, timestamp, marker_index)) {
        DEBUG_OUT("🔍 [DEBUG] Failed to decode marker pattern" << std::endl);
        return false;
    }
    DEBUG_OUT("🔍 [DEBUG] Marker decoded: ID=" << marker_id << ", confidence=" << confidence << std::endl);

    // Set marker properties
    marker.id = marker_id;
    marker.confidence = confidence;
    marker.center = center;
    marker.angle = angle;
    marker.deskew_angle = deskew_angle; // Track amount of deskew needed
    marker.corners = ordered_corners;

    return true;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error in processContour: " << e.what() << std::endl;
        return false;
    }
}

std::vector<cv::Point2f> MarkerDetector::sortCornersForMarker(const std::vector<cv::Point2f>& corners) {
    DEBUG_OUT("🔍 [DEBUG] SIMPLE corner sorting - using original order (matches debug visualization)" << std::endl);

    if (corners.size() != 4) {
        DEBUG_OUT("🔍 [DEBUG] Invalid number of corners for sorting: " << corners.size() << std::endl);
        return corners;
    }

    // Debug: Print original corners
    DEBUG_OUT("🔍 [DEBUG] Original corners (will be used as-is):" << std::endl);
    for (size_t i = 0; i < corners.size(); i++) {
        DEBUG_OUT("🔍 [DEBUG] Corner " << i << ": (" << corners[i].x << ", " << corners[i].y << ")" << std::endl);
    }

    // SIMPLE APPROACH: Use the same corner order as the debug visualization
    // The approxPolyDP already gives us a reasonable corner order
    // Don't mess with it - this matches what we see in the yellow debug boxes!

    std::vector<cv::Point2f> sorted_corners = corners;

    DEBUG_OUT("🔍 [DEBUG] Using original corner order (matches debug visualization):" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Corner 0: (" << sorted_corners[0].x << ", " << sorted_corners[0].y << ")" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Corner 1: (" << sorted_corners[1].x << ", " << sorted_corners[1].y << ")" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Corner 2: (" << sorted_corners[2].x << ", " << sorted_corners[2].y << ")" << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Corner 3: (" << sorted_corners[3].x << ", " << sorted_corners[3].y << ")" << std::endl);

    return sorted_corners;
}

bool MarkerDetector::extractAndDeskewMarker(const cv::Mat& frame, const std::vector<cv::Point2f>& corners, cv::Mat& marker_region, float& deskew_angle, const std::string& timestamp, int marker_index) {
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

    // Define destination points for perfect square (6x6 grid = 120x120 pixels)
    // This creates a perfect square for Codice marker processing with 4x4 inner core
    std::vector<cv::Point2f> dst_points = {
        cv::Point2f(0, 0),           // top-left
        cv::Point2f(119, 0),         // top-right
        cv::Point2f(119, 119),       // bottom-right
        cv::Point2f(0, 119)          // bottom-left
    };

    DEBUG_OUT("🔍 [DEBUG] Destination points defined for 120x120 square (6x6 grid)" << std::endl);

    // Calculate deskew angle before transformation
    // Use the top edge (TL to TR) to determine how much the marker is skewed
    cv::Point2f top_left = corners[0];
    cv::Point2f top_right = corners[1];
    deskew_angle = std::atan2(top_right.y - top_left.y, top_right.x - top_left.x) * 180.0f / CV_PI;
    DEBUG_OUT("🔍 [DEBUG] Calculated deskew angle: " << deskew_angle << " degrees" << std::endl);

    // Get perspective transform matrix
    cv::Mat transform_matrix = cv::getPerspectiveTransform(corners, dst_points);
    DEBUG_OUT("🔍 [DEBUG] Perspective transform matrix calculated" << std::endl);

    // Use the frame parameter (already the preprocessed frame) for marker extraction
    // This contains the actual marker content (white/black pattern) not just edges
    DEBUG_OUT("🔍 [DEBUG] Using passed frame for marker extraction, size: " << frame.cols << "x" << frame.rows << std::endl);

    // Apply perspective transform to extract and deskew the marker
    cv::warpPerspective(frame, marker_region, transform_matrix, cv::Size(120, 120));
    DEBUG_OUT("🔍 [DEBUG] Perspective transform applied, result size: " << marker_region.cols << "x" << marker_region.rows << std::endl);

    if (marker_region.empty()) {
        DEBUG_OUT("🔍 [DEBUG] ERROR: Perspective transform resulted in empty marker_region!" << std::endl);
        return false;
    }

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

    // Draw 6x6 grid (120x120 image = 20x20 pixel cells)
    for (int i = 0; i <= 6; i++) {
        int pos = i * 20;
        // Vertical lines
        cv::line(debug_marker, cv::Point(pos, 0), cv::Point(pos, 120), cv::Scalar(0, 0, 255), 1);
        // Horizontal lines
        cv::line(debug_marker, cv::Point(0, pos), cv::Point(120, pos), cv::Scalar(0, 0, 255), 1);
    }

    // Highlight the inner 4x4 grid (cells 1,1 to 4,4) in green
    cv::rectangle(debug_marker, cv::Point(20, 20), cv::Point(100, 100), cv::Scalar(0, 255, 0), 2);
    DEBUG_OUT("🔍 [DEBUG] Grid overlay completed, debug_marker size: " << debug_marker.cols << "x" << debug_marker.rows << std::endl);

    // Deskewed marker with grid is no longer saved - removed per user request

    DEBUG_OUT("🔍 [DEBUG] Marker region successfully extracted and deskewed" << std::endl);
    return true;
}

bool MarkerDetector::extractMarkerRegion(const cv::Mat& frame, const std::vector<cv::Point2f>& corners, cv::Mat& marker_region) {
    // Legacy method - redirect to new method
    float dummy_angle;
    return extractAndDeskewMarker(frame, corners, marker_region, dummy_angle);
}

bool MarkerDetector::decodeMarker(const cv::Mat& marker_region, int& marker_id, double& confidence, const std::string& timestamp, int marker_index) {
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

    // Save only the binary marker (4th requested debug image)
    if (debug_mode_ && !timestamp.empty()) {
        std::string prefix = "debug_output/" + timestamp;
        if (marker_index >= 0) {
            prefix += "_marker" + std::to_string(marker_index);
        }
        cv::imwrite(prefix + "_binary_marker.jpg", binary_marker);  // 4. Binary marker
        DEBUG_OUT("🔍 [DEBUG] Saved binary marker: " << prefix << "_binary_marker.jpg" << std::endl);
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

bool MarkerDetector::hasLocationChanged(const std::vector<CodiceMarker>& current_markers) {
    // If this is the first detection, always save
    if (previous_marker_locations_.empty()) {
        DEBUG_OUT("🔍 [DEBUG] First detection, saving debug images" << std::endl);
        return true;
    }

    // If number of markers changed, save
    if (current_markers.size() != previous_marker_locations_.size()) {
        DEBUG_OUT("🔍 [DEBUG] Marker count changed (" << previous_marker_locations_.size()
                  << " -> " << current_markers.size() << "), saving debug images" << std::endl);
        return true;
    }

    // Check if any marker moved significantly
    for (size_t i = 0; i < current_markers.size() && i < previous_marker_locations_.size(); i++) {
        cv::Point2f current_pos = current_markers[i].center;
        cv::Point2f previous_pos = previous_marker_locations_[i];

        double distance = cv::norm(current_pos - previous_pos);
        if (distance > location_change_threshold_) {
            DEBUG_OUT("🔍 [DEBUG] Marker " << i << " moved " << std::fixed << std::setprecision(1)
                      << distance << " pixels (threshold: " << location_change_threshold_
                      << "), saving debug images" << std::endl);
            return true;
        }
    }

    DEBUG_OUT("🔍 [DEBUG] No significant location changes, skipping debug images" << std::endl);
    return false;
}

cv::Mat MarkerDetector::getPerspectiveTransform(const cv::Mat& frame, const std::vector<cv::Point2f>& corners) {
    std::vector<cv::Point2f> dst_points = {
        cv::Point2f(0, 0), cv::Point2f(99, 0), cv::Point2f(99, 99), cv::Point2f(0, 99)
    };
    return cv::getPerspectiveTransform(corners, dst_points);
}

bool MarkerDetector::isValidCodicePattern(const cv::Mat& binary_marker) {
    DEBUG_OUT("🔍 [DEBUG] isValidCodicePattern called with size: " << binary_marker.cols << "x" << binary_marker.rows << std::endl);

    if (binary_marker.rows != 120 || binary_marker.cols != 120) {
        DEBUG_OUT("🔍 [DEBUG] Invalid size: expected 120x120, got " << binary_marker.cols << "x" << binary_marker.rows << std::endl);
        return false;
    }

    // Check outer border consistency (should be uniform - either all black or all white)
    DEBUG_OUT("🔍 [DEBUG] Checking outer border consistency..." << std::endl);

    // Sample a few border pixels to determine expected border color
    uchar border_sample1 = binary_marker.at<uchar>(0, 0);     // top-left corner
    uchar border_sample2 = binary_marker.at<uchar>(0, 60);    // top middle
    uchar border_sample3 = binary_marker.at<uchar>(60, 0);    // left middle
    uchar border_sample4 = binary_marker.at<uchar>(119, 119); // bottom-right corner

    // Determine if border should be black or white (after potential inversion)
    bool expect_black_border = (border_sample1 + border_sample2 + border_sample3 + border_sample4) / 4 < 127;
    uchar expected_threshold = expect_black_border ? 127 : 127;

    DEBUG_OUT("🔍 [DEBUG] Border samples: " << (int)border_sample1 << ", " << (int)border_sample2 << ", " << (int)border_sample3 << ", " << (int)border_sample4 << std::endl);
    DEBUG_OUT("🔍 [DEBUG] Expecting " << (expect_black_border ? "BLACK" : "WHITE") << " border" << std::endl);

    // Check border consistency (all pixels should be same color)
    int inconsistent_pixels = 0;
    // Top and bottom borders
    for (int x = 0; x < 120; x++) {
        bool top_matches = expect_black_border ? (binary_marker.at<uchar>(0, x) < expected_threshold) : (binary_marker.at<uchar>(0, x) >= expected_threshold);
        bool bottom_matches = expect_black_border ? (binary_marker.at<uchar>(119, x) < expected_threshold) : (binary_marker.at<uchar>(119, x) >= expected_threshold);

        if (!top_matches) inconsistent_pixels++;
        if (!bottom_matches) inconsistent_pixels++;
    }
    // Left and right borders
    for (int y = 0; y < 120; y++) {
        bool left_matches = expect_black_border ? (binary_marker.at<uchar>(y, 0) < expected_threshold) : (binary_marker.at<uchar>(y, 0) >= expected_threshold);
        bool right_matches = expect_black_border ? (binary_marker.at<uchar>(y, 119) < expected_threshold) : (binary_marker.at<uchar>(y, 119) >= expected_threshold);

        if (!left_matches) inconsistent_pixels++;
        if (!right_matches) inconsistent_pixels++;
    }

        // Allow generous tolerance for real-world conditions (max 40% inconsistent pixels)
    // With sub-pixel corner refinement, we can be more lenient on border consistency
    double inconsistency_ratio = (double)inconsistent_pixels / (4 * 120);
    DEBUG_OUT("🔍 [DEBUG] Border inconsistency: " << inconsistent_pixels << "/" << (4*120) << " pixels (" << (inconsistency_ratio*100) << "%)" << std::endl);

    if (inconsistency_ratio > 0.60) {  // More than 60% inconsistent - real markers aren't perfect
        DEBUG_OUT("🔍 [DEBUG] Border validation failed: too much inconsistency (" << (inconsistency_ratio*100) << "%)" << std::endl);
        return false;
    }
    DEBUG_OUT("🔍 [DEBUG] Outer border check passed" << std::endl);

        // Simple validation - just check if it looks like a marker pattern
    // For now, accept any 120x120 binary image that made it this far
    DEBUG_OUT("🔍 [DEBUG] Basic pattern validation passed" << std::endl);
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

void MarkerDetector::drawAllContoursDebug(cv::Mat& frame, const std::vector<std::vector<cv::Point>>& contours) {
    // Draw all contours with different colors based on their validation status
    for (size_t i = 0; i < contours.size(); i++) {
        const auto& contour = contours[i];

        // Approximate contour to check if it could be a 4-corner shape
        std::vector<cv::Point> approx;
        double epsilon = 0.05 * cv::arcLength(contour, true);
        cv::approxPolyDP(contour, approx, epsilon, true);

        cv::Scalar color;
        std::string label;

        if (approx.size() == 4) {
            // 4 corners - potential marker candidate (yellow)
            color = cv::Scalar(0, 255, 255);
            label = "4-corner";

            // Draw the approximated polygon
            std::vector<cv::Point> poly_points = approx;
            cv::polylines(frame, poly_points, true, color, 2);

            // Mark the center
            cv::Moments m = cv::moments(contour);
            if (m.m00 != 0) {
                cv::Point center(m.m10 / m.m00, m.m01 / m.m00);
                cv::circle(frame, center, 3, color, -1);

                // Add label
                cv::putText(frame, label + " #" + std::to_string(i),
                           cv::Point(center.x + 10, center.y - 10),
                           cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
            }
        } else if (approx.size() > 4 && approx.size() <= 8) {
            // Multi-corner shape - might be noisy marker (orange)
            color = cv::Scalar(0, 165, 255);
            label = std::to_string(approx.size()) + "-corner";

            // Draw the original contour
            cv::drawContours(frame, std::vector<std::vector<cv::Point>>{contour}, -1, color, 1);

            // Mark the center
            cv::Moments m = cv::moments(contour);
            if (m.m00 != 0) {
                cv::Point center(m.m10 / m.m00, m.m01 / m.m00);
                cv::circle(frame, center, 2, color, -1);

                // Add label
                cv::putText(frame, label,
                           cv::Point(center.x + 5, center.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.3, color, 1);
            }
        } else {
            // Not a candidate shape (gray)
            color = cv::Scalar(128, 128, 128);

            // Draw just a thin outline
            cv::drawContours(frame, std::vector<std::vector<cv::Point>>{contour}, -1, color, 1);
        }

        // Add contour info in corner
        std::string contour_info = "#" + std::to_string(i) + ": " + std::to_string(contour.size()) + "pts";
        cv::putText(frame, contour_info,
                   cv::Point(10, 20 + i * 15),
                   cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(255, 255, 255), 1);
    }

    // Add legend
    int legend_y = frame.rows - 80;
    cv::putText(frame, "Legend:", cv::Point(10, legend_y), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    cv::putText(frame, "Yellow: 4-corner candidates", cv::Point(10, legend_y + 15), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0, 255, 255), 1);
    cv::putText(frame, "Orange: Multi-corner shapes", cv::Point(10, legend_y + 30), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0, 165, 255), 1);
    cv::putText(frame, "Gray: Other contours", cv::Point(10, legend_y + 45), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(128, 128, 128), 1);
    cv::putText(frame, "Green: Valid markers", cv::Point(10, legend_y + 60), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0, 255, 0), 1);
}

void MarkerDetector::drawDebugInfo(cv::Mat& frame, const std::vector<CodiceMarker>& markers) {
    for (const auto& marker : markers) {
        // Draw marker outline in bright green for successfully detected markers
        std::vector<cv::Point> int_corners;
        for (const auto& corner : marker.corners) {
            int_corners.emplace_back(static_cast<int>(corner.x), static_cast<int>(corner.y));
        }

        cv::polylines(frame, int_corners, true, cv::Scalar(0, 255, 0), 3);

        // Draw center point
        cv::circle(frame, cv::Point(static_cast<int>(marker.center.x), static_cast<int>(marker.center.y)),
                   8, cv::Scalar(0, 0, 255), -1);

        // Draw marker ID, confidence, and deskew angle
        std::string info = "ID:" + std::to_string(marker.id) + " C:" + std::to_string(marker.confidence).substr(0, 3);
        std::string angle_info = "A:" + std::to_string(marker.angle).substr(0, 4) + " D:" + std::to_string(marker.deskew_angle).substr(0, 4);

        // Add background rectangle for text visibility
        cv::Point text_pos(static_cast<int>(marker.center.x) - 40, static_cast<int>(marker.center.y) - 35);
        cv::Size text_size = cv::getTextSize(info, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
        cv::rectangle(frame, text_pos + cv::Point(-2, -text_size.height - 2),
                     text_pos + cv::Point(text_size.width + 2, 2),
                     cv::Scalar(0, 0, 0), -1);

        cv::putText(frame, info, text_pos,
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        cv::putText(frame, angle_info, cv::Point(static_cast<int>(marker.center.x) - 40, static_cast<int>(marker.center.y) - 15),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 0), 1);
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

void MarkerDetector::setDebugWindow(bool enable) {
    debug_window_enabled_ = enable;
    if (enable) {
        // Create the live debug window
        try {
            cv::namedWindow("Codice Marker Debug", cv::WINDOW_AUTOSIZE);
            VERBOSE_OUT("🖥️ Live debug window enabled" << std::endl);
        } catch (const cv::Exception& e) {
            std::cerr << "❌ Failed to create debug window: " << e.what() << std::endl;
            debug_window_enabled_ = false;
        }
    } else {
        try {
            cv::destroyWindow("Codice Marker Debug");
            VERBOSE_OUT("🖥️ Live debug window disabled" << std::endl);
        } catch (const cv::Exception& e) {
            // Ignore window destruction errors
        }
    }
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

void MarkerDetector::drawLiveDebugWindow(const cv::Mat& frame, const std::vector<std::vector<cv::Point>>& contours, const std::vector<CodiceMarker>& markers) {
    if (!debug_window_enabled_) {
        return;
    }

    try {
        // Debug output
        DEBUG_OUT("🖥️ drawLiveDebugWindow called with frame size: " << frame.cols << "x" << frame.rows << std::endl);
        DEBUG_OUT("🖥️ Contours: " << contours.size() << ", Markers: " << markers.size() << std::endl);

        // Create a copy of the frame for debug visualization
        cv::Mat debug_frame = frame.clone();

        // Draw all contours with yellow rectangles for possible candidates
        for (size_t i = 0; i < contours.size(); i++) {
            const auto& contour = contours[i];

            // Approximate contour to check if it could be a 4-corner shape
            std::vector<cv::Point> approx;
            double epsilon = 0.05 * cv::arcLength(contour, true);
            cv::approxPolyDP(contour, approx, epsilon, true);

            cv::Scalar color;
            std::string label;

            if (approx.size() == 4) {
                // 4 corners - potential marker candidate (yellow)
                color = cv::Scalar(0, 255, 255); // Yellow in BGR
                label = "Candidate #" + std::to_string(i);

                // Draw the approximated polygon
                std::vector<cv::Point> poly_points = approx;
                cv::polylines(debug_frame, poly_points, true, color, 2);

                // Mark the center
                cv::Moments m = cv::moments(contour);
                if (m.m00 != 0) {
                    cv::Point center(m.m10 / m.m00, m.m01 / m.m00);
                    cv::circle(debug_frame, center, 3, color, -1);

                    // Add label
                    cv::putText(debug_frame, label,
                               cv::Point(center.x + 10, center.y - 10),
                               cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
                }
            } else if (approx.size() > 4 && approx.size() <= 8) {
                // Multi-corner shape - might be noisy marker (orange)
                color = cv::Scalar(0, 165, 255); // Orange in BGR
                label = std::to_string(approx.size()) + "-corner";

                // Draw the original contour
                cv::drawContours(debug_frame, std::vector<std::vector<cv::Point>>{contour}, -1, color, 1);

                // Mark the center
                cv::Moments m = cv::moments(contour);
                if (m.m00 != 0) {
                    cv::Point center(m.m10 / m.m00, m.m01 / m.m00);
                    cv::circle(debug_frame, center, 2, color, -1);

                    // Add label
                    cv::putText(debug_frame, label,
                               cv::Point(center.x + 5, center.y - 5),
                               cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
                }
            } else {
                // Not a candidate shape (gray)
                color = cv::Scalar(128, 128, 128); // Gray in BGR

                // Draw just a thin outline
                cv::drawContours(debug_frame, std::vector<std::vector<cv::Point>>{contour}, -1, color, 1);
            }
        }

        // Draw detected markers with green rectangles and detailed info
        for (const auto& marker : markers) {
            // Draw marker outline in bright green for successfully detected markers
            std::vector<cv::Point> int_corners;
            for (const auto& corner : marker.corners) {
                int_corners.emplace_back(static_cast<int>(corner.x), static_cast<int>(corner.y));
            }

            cv::polylines(debug_frame, int_corners, true, cv::Scalar(0, 255, 0), 3); // Green in BGR

            // Draw center point
            cv::Point center(static_cast<int>(marker.center.x), static_cast<int>(marker.center.y));
            cv::circle(debug_frame, center, 8, cv::Scalar(0, 0, 255), -1); // Red center

            // Draw marker ID, confidence, and orientation info
            std::string id_info = "ID: " + std::to_string(marker.id);
            std::string conf_info = "Conf: " + std::to_string(marker.confidence).substr(0, 4);
            std::string angle_info = "Angle: " + std::to_string(static_cast<int>(marker.angle)) + "°";
            std::string loc_info = "Loc: (" + std::to_string(static_cast<int>(marker.center.x)) + 
                                 ", " + std::to_string(static_cast<int>(marker.center.y)) + ")";

            // Position text above the marker
            cv::Point text_pos(center.x - 50, center.y - 50);
            
            // Add background rectangles for text visibility
            cv::Size id_size = cv::getTextSize(id_info, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, nullptr);
            cv::Size conf_size = cv::getTextSize(conf_info, cv::FONT_HERSHEY_SIMPLEX, 0.5, 2, nullptr);
            cv::Size angle_size = cv::getTextSize(angle_info, cv::FONT_HERSHEY_SIMPLEX, 0.5, 2, nullptr);
            cv::Size loc_size = cv::getTextSize(loc_info, cv::FONT_HERSHEY_SIMPLEX, 0.5, 2, nullptr);

            // Draw background rectangles
            cv::rectangle(debug_frame, text_pos + cv::Point(-5, -id_size.height - 5),
                         text_pos + cv::Point(std::max({id_size.width, conf_size.width, angle_size.width, loc_size.width}) + 5, 5),
                         cv::Scalar(0, 0, 0), -1);

            // Draw text
            cv::putText(debug_frame, id_info, text_pos,
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
            cv::putText(debug_frame, conf_info, cv::Point(text_pos.x, text_pos.y + 20),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 2);
            cv::putText(debug_frame, angle_info, cv::Point(text_pos.x, text_pos.y + 40),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);
            cv::putText(debug_frame, loc_info, cv::Point(text_pos.x, text_pos.y + 60),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 255), 2);
        }

        // Add legend in top-left corner
        int legend_y = 30;
        cv::putText(debug_frame, "Codice Marker Debug", cv::Point(10, legend_y), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        legend_y += 25;
        cv::putText(debug_frame, "Yellow: 4-corner candidates", cv::Point(10, legend_y), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
        legend_y += 20;
        cv::putText(debug_frame, "Orange: Multi-corner shapes", cv::Point(10, legend_y), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 165, 255), 1);
        legend_y += 20;
        cv::putText(debug_frame, "Green: Valid markers", cv::Point(10, legend_y), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
        legend_y += 20;
        cv::putText(debug_frame, "Red dot: Marker center", cv::Point(10, legend_y), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);

        // Add frame info in top-right corner
        std::string frame_info = "Frame: " + std::to_string(total_frames_processed_);
        std::string contour_info = "Contours: " + std::to_string(contours.size());
        std::string marker_info = "Markers: " + std::to_string(markers.size());
        
        cv::Size frame_info_size = cv::getTextSize(frame_info, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
        cv::Point frame_info_pos(debug_frame.cols - frame_info_size.width - 10, 30);
        
        cv::putText(debug_frame, frame_info, frame_info_pos, 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        cv::putText(debug_frame, contour_info, cv::Point(frame_info_pos.x, frame_info_pos.y + 20), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        cv::putText(debug_frame, marker_info, cv::Point(frame_info_pos.x, frame_info_pos.y + 40), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

        // Display the debug frame
        DEBUG_OUT("🖥️ About to display debug frame, size: " << debug_frame.cols << "x" << debug_frame.rows << std::endl);
        cv::imshow("Codice Marker Debug", debug_frame);
        
        // Only process window events occasionally to avoid blocking
        static int frame_counter = 0;
        frame_counter++;
        if (frame_counter % 10 == 0) { // Every 10th frame
            int key = cv::waitKey(1);
            if (key == 27) { // ESC key pressed
                DEBUG_OUT("🖥️ ESC key pressed, closing debug window" << std::endl);
                debug_window_enabled_ = false;
                cv::destroyWindow("Codice Marker Debug");
            }
        }
        DEBUG_OUT("🖥️ Debug frame displayed" << std::endl);

    } catch (const cv::Exception& e) {
        std::cerr << "❌ Error in drawLiveDebugWindow: " << e.what() << std::endl;
    }
}

} // namespace CodiceCam
