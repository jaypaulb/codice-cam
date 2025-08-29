#include "ImageProcessor.h"
#include <iostream>
#include <algorithm>

namespace CodiceCam {

ImageProcessor::ImageProcessor()
    : blur_kernel_size_(5)
    , contrast_alpha_(1.2)
    , brightness_beta_(10)
    , canny_low_threshold_(50)
    , canny_high_threshold_(150)
    , min_contour_area_(1000)
    , max_contour_area_(50000)
    , min_contour_perimeter_(100)
{
}

ImageProcessor::~ImageProcessor() {
}

bool ImageProcessor::processFrame(const cv::Mat& input_frame, cv::Mat& processed_frame) {
    if (input_frame.empty()) {
        std::cerr << "❌ Input frame is empty" << std::endl;
        return false;
    }

    if (!validateParameters()) {
        std::cerr << "❌ Invalid preprocessing parameters" << std::endl;
        return false;
    }

    try {
        // Step 1: Preprocess the frame (grayscale, blur, contrast)
        cv::Mat preprocessed = preprocessFrame(input_frame);

        // Step 2: Detect edges
        cv::Mat edges = detectEdges(preprocessed);

        // Store the processed frame (edges for contour detection)
        processed_frame = edges.clone();

        // Store the preprocessed frame for pattern reading
        preprocessed_frame_ = preprocessed.clone();

        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "❌ OpenCV error in processFrame: " << e.what() << std::endl;
        return false;
    }
}

bool ImageProcessor::findMarkerContours(const cv::Mat& processed_frame, std::vector<std::vector<cv::Point>>& contours) {
    if (processed_frame.empty()) {
        std::cerr << "❌ Processed frame is empty" << std::endl;
        return false;
    }

    try {
        // Find all contours
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(processed_frame, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        std::cout << "🔍 [DEBUG] Found " << contours.size() << " raw contours" << std::endl;

        // Limit the number of contours to process to prevent hanging
        const int MAX_CONTOURS_TO_PROCESS = 1000;
        if (contours.size() > MAX_CONTOURS_TO_PROCESS) {
            std::cout << "⚠️ [DEBUG] Too many contours (" << contours.size() << "), limiting to " << MAX_CONTOURS_TO_PROCESS << std::endl;
            contours.resize(MAX_CONTOURS_TO_PROCESS);
        }

        // Filter contours based on size and shape criteria
        std::vector<std::vector<cv::Point>> filtered_contours;
        int processed_count = 0;
        for (const auto& contour : contours) {
            processed_count++;
            if (processed_count % 100 == 0) {
                std::cout << "🔍 [DEBUG] Processed " << processed_count << "/" << contours.size() << " contours" << std::endl;
            }
            if (filterContour(contour)) {
                filtered_contours.push_back(contour);
            }
        }

        contours = std::move(filtered_contours);

        if (contours.size() > 0) {
            // Removed debug output - this was showing false positives in minimal mode
        }
        return !contours.empty();

    } catch (const cv::Exception& e) {
        std::cerr << "❌ OpenCV error in findMarkerContours: " << e.what() << std::endl;
        return false;
    }
}

void ImageProcessor::setPreprocessingParams(int blur_kernel_size, double contrast_alpha, int brightness_beta) {
    blur_kernel_size_ = blur_kernel_size;
    contrast_alpha_ = contrast_alpha;
    brightness_beta_ = brightness_beta;

    std::cout << "⚙️ Preprocessing params updated: blur=" << blur_kernel_size_
              << ", contrast=" << contrast_alpha_ << ", brightness=" << brightness_beta_ << std::endl;
}

void ImageProcessor::setEdgeDetectionParams(int low_threshold, int high_threshold) {
    canny_low_threshold_ = low_threshold;
    canny_high_threshold_ = high_threshold;

    std::cout << "⚙️ Edge detection params updated: low=" << canny_low_threshold_
              << ", high=" << canny_high_threshold_ << std::endl;
}

void ImageProcessor::setContourFilterParams(double min_area, double max_area, double min_perimeter) {
    min_contour_area_ = min_area;
    max_contour_area_ = max_area;
    min_contour_perimeter_ = min_perimeter;

    std::cout << "⚙️ Contour filter params updated: area=[" << min_contour_area_
              << "," << max_contour_area_ << "], min_perimeter=" << min_contour_perimeter_ << std::endl;
}

std::string ImageProcessor::getParameterInfo() const {
    std::string info = "ImageProcessor Parameters:\n";
    info += "  Preprocessing: blur=" + std::to_string(blur_kernel_size_) +
            ", contrast=" + std::to_string(contrast_alpha_) +
            ", brightness=" + std::to_string(brightness_beta_) + "\n";
    info += "  Edge Detection: low=" + std::to_string(canny_low_threshold_) +
            ", high=" + std::to_string(canny_high_threshold_) + "\n";
    info += "  Contour Filter: area=[" + std::to_string(min_contour_area_) +
            "," + std::to_string(max_contour_area_) +
            "], min_perimeter=" + std::to_string(min_contour_perimeter_);
    return info;
}

cv::Mat ImageProcessor::preprocessFrame(const cv::Mat& input_frame) {
    cv::Mat processed;

    // Convert to grayscale
    if (input_frame.channels() == 3) {
        cv::cvtColor(input_frame, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = input_frame.clone();
    }

    // Apply Gaussian blur to reduce noise
    if (blur_kernel_size_ > 0) {
        cv::GaussianBlur(processed, processed, cv::Size(blur_kernel_size_, blur_kernel_size_), 0);
    }

    // Enhance contrast and brightness
    if (contrast_alpha_ != 1.0 || brightness_beta_ != 0) {
        processed.convertTo(processed, -1, contrast_alpha_, brightness_beta_);
    }

    return processed;
}

cv::Mat ImageProcessor::detectEdges(const cv::Mat& grayscale_frame) {
    cv::Mat edges;

    // Apply Canny edge detection
    cv::Canny(grayscale_frame, edges, canny_low_threshold_, canny_high_threshold_);

    // Apply minimal morphological operations to preserve square corners
    // Use smaller kernel to close small gaps without distorting square shapes
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel, cv::Point(-1,-1), 1);

    return edges;
}

bool ImageProcessor::filterContour(const std::vector<cv::Point>& contour) const {
    if (contour.size() < 4) {
        return false; // Need at least 4 points for a quadrilateral
    }

    // Calculate contour area
    double area = cv::contourArea(contour);
    if (area < min_contour_area_ || area > max_contour_area_) {
        return false;
    }

    // Calculate contour perimeter
    double perimeter = cv::arcLength(contour, true);
    if (perimeter < min_contour_perimeter_) {
        return false;
    }

    // Check if contour is roughly rectangular (for Codice markers)
    std::vector<cv::Point> approx;
    cv::approxPolyDP(contour, approx, 0.02 * perimeter, true);

    // Codice markers are ALWAYS perfect squares with EXACTLY 4 corners
    if (approx.size() != 4) {
        return false; // Reject all multi-corner shapes - markers are always square
    }

    // Check aspect ratio (must be very close to square)
    cv::Rect bounding_rect = cv::boundingRect(contour);
    double aspect_ratio = static_cast<double>(bounding_rect.width) / bounding_rect.height;
    if (aspect_ratio < 0.8 || aspect_ratio > 1.25) {
        return false; // Much stricter square requirement
    }

    // Additional square validation: check corner angles
    // For a proper square, internal angles should be close to 90 degrees
    for (size_t i = 0; i < 4; i++) {
        cv::Point p1 = approx[i];
        cv::Point p2 = approx[(i + 1) % 4];
        cv::Point p3 = approx[(i + 2) % 4];

        // Calculate vectors
        cv::Point v1 = p1 - p2;
        cv::Point v2 = p3 - p2;

        // Calculate angle using dot product
        double dot = v1.x * v2.x + v1.y * v2.y;
        double mag1 = sqrt(v1.x * v1.x + v1.y * v1.y);
        double mag2 = sqrt(v2.x * v2.x + v2.y * v2.y);

        if (mag1 > 0 && mag2 > 0) {
            double angle = acos(std::abs(dot) / (mag1 * mag2)) * 180.0 / CV_PI;
            // Allow some tolerance for real-world conditions (70-110 degrees)
            if (angle < 70 || angle > 110) {
                return false; // Corner angle too far from 90 degrees
            }
        }
    }

    return true;
}

bool ImageProcessor::validateParameters() const {
    // Validate blur kernel size (must be odd and positive)
    if (blur_kernel_size_ < 0 || (blur_kernel_size_ > 0 && blur_kernel_size_ % 2 == 0)) {
        return false;
    }

    // Validate contrast and brightness
    if (contrast_alpha_ <= 0) {
        return false;
    }

    // Validate edge detection thresholds
    if (canny_low_threshold_ < 0 || canny_high_threshold_ < 0 ||
        canny_low_threshold_ >= canny_high_threshold_) {
        return false;
    }

    // Validate contour filter parameters
    if (min_contour_area_ < 0 || max_contour_area_ < 0 ||
        min_contour_area_ >= max_contour_area_ || min_contour_perimeter_ < 0) {
        return false;
    }

    return true;
}

const cv::Mat& ImageProcessor::getPreprocessedFrame() const {
    return preprocessed_frame_;
}

} // namespace CodiceCam
