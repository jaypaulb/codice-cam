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

        // Filter contours based on size and shape criteria
        std::vector<std::vector<cv::Point>> filtered_contours;
        for (const auto& contour : contours) {
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

    // Apply morphological operations to close gaps
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);

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

    // Codice markers should be roughly square/rectangular (4 corners)
    if (approx.size() < 4 || approx.size() > 8) {
        return false;
    }

    // Check aspect ratio (should be roughly square)
    cv::Rect bounding_rect = cv::boundingRect(contour);
    double aspect_ratio = static_cast<double>(bounding_rect.width) / bounding_rect.height;
    if (aspect_ratio < 0.5 || aspect_ratio > 2.0) {
        return false;
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
