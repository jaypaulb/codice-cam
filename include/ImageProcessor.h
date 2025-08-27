#pragma once

#include <opencv2/opencv.hpp>
#include <memory>
#include <vector>

namespace CodiceCam {

/**
 * @brief Processes camera frames for Codice marker detection
 *
 * The ImageProcessor handles image preprocessing steps including:
 * - Grayscale conversion
 * - Noise reduction
 * - Contrast enhancement
 * - Edge detection
 * - Contour finding
 */
class ImageProcessor {
public:
    /**
     * @brief Constructor
     */
    ImageProcessor();

    /**
     * @brief Destructor
     */
    ~ImageProcessor();

    /**
     * @brief Process a frame for marker detection
     * @param input_frame Input color frame from camera
     * @param processed_frame Output processed frame
     * @return true if processing successful, false otherwise
     */
    bool processFrame(const cv::Mat& input_frame, cv::Mat& processed_frame);

    /**
     * @brief Find potential marker contours in processed frame
     * @param processed_frame Preprocessed frame
     * @param contours Output vector of contours
     * @return true if contours found, false otherwise
     */
    bool findMarkerContours(const cv::Mat& processed_frame, std::vector<std::vector<cv::Point>>& contours);

    /**
     * @brief Set preprocessing parameters
     * @param blur_kernel_size Gaussian blur kernel size (must be odd)
     * @param contrast_alpha Contrast enhancement factor (1.0 = no change)
     * @param brightness_beta Brightness offset (0 = no change)
     */
    void setPreprocessingParams(int blur_kernel_size = 5, double contrast_alpha = 1.2, int brightness_beta = 10);

    /**
     * @brief Set edge detection parameters
     * @param low_threshold Canny edge detection low threshold
     * @param high_threshold Canny edge detection high threshold
     */
    void setEdgeDetectionParams(int low_threshold = 50, int high_threshold = 150);

    /**
     * @brief Set contour filtering parameters
     * @param min_area Minimum contour area
     * @param max_area Maximum contour area
     * @param min_perimeter Minimum contour perimeter
     */
    void setContourFilterParams(double min_area = 1000, double max_area = 50000, double min_perimeter = 100);

    /**
     * @brief Get current preprocessing parameters
     * @return String description of current parameters
     */
    std::string getParameterInfo() const;

    /**
     * @brief Get the preprocessed frame (for pattern reading)
     * @return Reference to the preprocessed frame
     */
    const cv::Mat& getPreprocessedFrame() const;

private:
    // Preprocessing parameters
    int blur_kernel_size_;
    double contrast_alpha_;
    int brightness_beta_;

    // Edge detection parameters
    int canny_low_threshold_;
    int canny_high_threshold_;

    // Contour filtering parameters
    double min_contour_area_;
    double max_contour_area_;
    double min_contour_perimeter_;

    // Store preprocessed frame for pattern reading
    cv::Mat preprocessed_frame_;

    // Internal processing methods
    cv::Mat preprocessFrame(const cv::Mat& input_frame);
    cv::Mat detectEdges(const cv::Mat& grayscale_frame);
    bool filterContour(const std::vector<cv::Point>& contour) const;

    /**
     * @brief Validate preprocessing parameters
     * @return true if parameters are valid, false otherwise
     */
    bool validateParameters() const;
};

} // namespace CodiceCam
