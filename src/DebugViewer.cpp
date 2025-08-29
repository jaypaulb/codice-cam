#include "DebugViewer.h"
#include <iostream>

namespace CodiceCam {

DebugViewer::DebugViewer()
    : window_initialized_(false)
    , window_name_("Codice Debug Viewer")
{
}

DebugViewer::~DebugViewer() {
    close();
}

bool DebugViewer::initialize() {
    try {
        cv::namedWindow(window_name_, cv::WINDOW_AUTOSIZE);
        window_initialized_ = true;
        std::cout << "🖥️ Debug viewer initialized successfully" << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "❌ Failed to initialize debug viewer: " << e.what() << std::endl;
        window_initialized_ = false;
        return false;
    }
}

bool DebugViewer::processFrame(const cv::Mat& frame, bool show_contours, bool show_edges) {
    if (!window_initialized_ || frame.empty()) {
        return false;
    }

    try {
        // Clone the frame for modifications
        cv::Mat display_frame = frame.clone();

        // Convert to color if needed
        if (display_frame.channels() == 1) {
            cv::cvtColor(display_frame, display_frame, cv::COLOR_GRAY2BGR);
        }

        // Show contours if requested
        if (show_contours) {
            auto contours = findSimpleContours(frame);
            drawContourOverlays(display_frame, contours);
            drawInfoOverlay(display_frame, 0, static_cast<int>(contours.size()));
        }

        // Show edges if requested
        if (show_edges) {
            cv::Mat edges = detectEdges(frame);
            drawEdgeOverlay(display_frame, edges);
        }

        // Store current frame for potential marker overlay
        current_frame_ = display_frame.clone();

        // Display the frame
        cv::imshow(window_name_, display_frame);

        // Non-blocking wait with ESC key handling
        int key = cv::waitKey(1);
        if (key == 27) { // ESC key
            std::cout << "🖥️ ESC pressed, closing debug viewer" << std::endl;
            close();
            return false;
        }

        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "❌ Error in processFrame: " << e.what() << std::endl;
        return false;
    }
}

void DebugViewer::addMarkerOverlays(const std::vector<CodiceMarker>& markers) {
    if (!window_initialized_ || current_frame_.empty()) {
        return;
    }

    try {
        cv::Mat overlay_frame = current_frame_.clone();

        for (const auto& marker : markers) {
            // Draw marker outline in green
            std::vector<cv::Point> corners;
            for (const auto& corner : marker.corners) {
                corners.emplace_back(static_cast<int>(corner.x), static_cast<int>(corner.y));
            }

            cv::polylines(overlay_frame, corners, true, cv::Scalar(0, 255, 0), 3); // Green

            // Draw center point
            cv::Point center(static_cast<int>(marker.center.x), static_cast<int>(marker.center.y));
            cv::circle(overlay_frame, center, 8, cv::Scalar(0, 0, 255), -1); // Red center

            // Draw marker info
            std::string id_text = "ID: " + std::to_string(marker.id);
            std::string conf_text = "Conf: " + std::to_string(marker.confidence).substr(0, 4);

            cv::Point text_pos(center.x - 40, center.y - 35);

            // Add background for text
            cv::Size id_size = cv::getTextSize(id_text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
            cv::Size conf_size = cv::getTextSize(conf_text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);

            cv::rectangle(overlay_frame, text_pos + cv::Point(-2, -id_size.height - 2),
                         text_pos + cv::Point(std::max(id_size.width, conf_size.width) + 2, 2),
                         cv::Scalar(0, 0, 0), -1);

            // Draw text
            cv::putText(overlay_frame, id_text, text_pos,
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
            cv::putText(overlay_frame, conf_text, cv::Point(text_pos.x, text_pos.y + 20),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
        }

        // Update display
        cv::imshow(window_name_, overlay_frame);
        cv::waitKey(1);

    } catch (const cv::Exception& e) {
        std::cerr << "❌ Error adding marker overlays: " << e.what() << std::endl;
    }
}

bool DebugViewer::isWindowOpen() const {
    return window_initialized_;
}

void DebugViewer::close() {
    if (window_initialized_) {
        try {
            cv::destroyWindow(window_name_);
        } catch (const cv::Exception& e) {
            // Ignore destruction errors
        }
        window_initialized_ = false;
        std::cout << "🖥️ Debug viewer closed" << std::endl;
    }
}

std::vector<std::vector<cv::Point>> DebugViewer::findSimpleContours(const cv::Mat& frame) {
    std::vector<std::vector<cv::Point>> contours;

    try {
        // Convert to grayscale if needed
        cv::Mat gray;
        if (frame.channels() == 3) {
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = frame.clone();
        }

        // Simple blur to reduce noise
        cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);

        // Simple threshold to get binary image
        cv::Mat binary;
        cv::threshold(gray, binary, 100, 255, cv::THRESH_BINARY);

        // Find contours (simple approach)
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // Filter contours - keep only those with reasonable size and 4 corners
        std::vector<std::vector<cv::Point>> filtered_contours;
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area > 100 && area < 50000) { // Reasonable size filter
                // Approximate to check for quadrilateral
                std::vector<cv::Point> approx;
                double perimeter = cv::arcLength(contour, true);
                cv::approxPolyDP(contour, approx, 0.02 * perimeter, true);

                if (approx.size() == 4) { // Only keep 4-corner shapes
                    filtered_contours.push_back(contour);
                }
            }
        }

        contours = std::move(filtered_contours);

    } catch (const cv::Exception& e) {
        std::cerr << "❌ Error in findSimpleContours: " << e.what() << std::endl;
        contours.clear();
    }

    return contours;
}

cv::Mat DebugViewer::detectEdges(const cv::Mat& frame) {
    cv::Mat edges;

    try {
        cv::Mat gray;
        if (frame.channels() == 3) {
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = frame.clone();
        }

        cv::Canny(gray, edges, 50, 150);
    } catch (const cv::Exception& e) {
        std::cerr << "❌ Error in detectEdges: " << e.what() << std::endl;
    }

    return edges;
}

void DebugViewer::drawContourOverlays(cv::Mat& frame, const std::vector<std::vector<cv::Point>>& contours) {
    for (size_t i = 0; i < contours.size(); i++) {
        // Draw contour in yellow
        cv::drawContours(frame, contours, static_cast<int>(i), cv::Scalar(0, 255, 255), 2);

        // Add small label
        if (!contours[i].empty()) {
            cv::Moments m = cv::moments(contours[i]);
            if (m.m00 != 0) {
                cv::Point center(m.m10 / m.m00, m.m01 / m.m00);
                std::string label = "#" + std::to_string(i);
                cv::putText(frame, label, cv::Point(center.x + 5, center.y - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0, 255, 255), 1);
            }
        }
    }
}

void DebugViewer::drawEdgeOverlay(cv::Mat& frame, const cv::Mat& edges) {
    // Overlay edges in red
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols; x++) {
            if (edges.at<uchar>(y, x) > 0) {
                frame.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255); // Red
            }
        }
    }
}

void DebugViewer::drawInfoOverlay(cv::Mat& frame, int frame_count, int contour_count) {
    // Add info in top-left
    std::string info1 = "Contours: " + std::to_string(contour_count);
    std::string info2 = "Yellow: 4-corner shapes";

    cv::putText(frame, info1, cv::Point(10, 20),
               cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    cv::putText(frame, info2, cv::Point(10, 40),
               cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 255), 1);
    cv::putText(frame, "Green: Valid markers", cv::Point(10, 55),
               cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 0), 1);
}

} // namespace CodiceCam
