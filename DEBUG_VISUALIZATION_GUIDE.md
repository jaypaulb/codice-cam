# Debug Visualization Guide

## Overview
The enhanced debug visualization system helps you tune marker detection parameters by showing exactly what the system is detecting on each frame.

## How to Use

### 1. Run the Configurable Test
```bash
./test_configurable_detection
```

### 2. Check Debug Images
The system generates several debug images in the **`debug_output/`** folder:

- **`debug_output/debug_frame.jpg`** - Main debug visualization showing all detection attempts
- **`debug_output/processed_frame.jpg`** - Edge-detected frame used for contour detection
- **`debug_output/preprocessed_frame.jpg`** - Grayscale/contrast-enhanced frame used for pattern extraction
- **`debug_output/deskewed_marker_with_grid.jpg`** - Extracted marker region with 6x6 grid overlay
- **`debug_output/binary_marker.jpg`** - Final binary marker used for pattern decoding
- **`debug_output/marker_region.jpg`** - Raw extracted marker region
- **`debug_output/gray_marker.jpg`** - Grayscale version before binarization

### 3. Understanding the Debug Frame

#### Color Coding:
- **🟡 Yellow outlines** = 4-corner candidates (potential markers)
- **🟠 Orange outlines** = Multi-corner shapes (5-8 corners, might be noisy detection)
- **⚫ Gray outlines** = Other contours (not marker candidates)
- **🟢 Green outlines** = Successfully validated markers (with thick border)

#### Information Displayed:
- **Top-left corner**: List of all contours with point counts
- **Bottom area**: Legend explaining color coding
- **On markers**: Contour numbers and corner counts
- **On valid markers**: ID, confidence, angle, and deskew information

### 4. Tuning Parameters

Edit `detection_config.txt` to adjust sensitivity:

#### To Reduce False Positives:
- Increase `min_contour_area` (reject smaller shapes)
- Increase `min_marker_size` (reject smaller markers)
- Increase `min_confidence` (be more strict about pattern matching)
- Decrease `canny_low_threshold` (detect fewer edges)

#### To Increase Detection Sensitivity:
- Decrease `min_contour_area` (detect smaller shapes)
- Decrease `min_marker_size` (accept smaller markers)
- Decrease `min_confidence` (be less strict about pattern matching)
- Increase `canny_high_threshold` (detect more edges)

#### Image Quality Adjustments:
- Increase `contrast_alpha` for better contrast (1.5-2.0)
- Adjust `brightness_beta` for lighting compensation (+/-50)
- Use `blur_kernel_size=3` or `5` to reduce noise (at cost of sharpness)

### 5. Interpreting Results

#### Good Detection:
- Few yellow 4-corner candidates
- Most yellow candidates become green valid markers
- Clean, square-shaped yellow outlines

#### Too Sensitive (Many False Positives):
- Many yellow 4-corner candidates
- Few become green valid markers
- Irregular, noisy yellow shapes

#### Too Restrictive:
- No yellow candidates even when markers are visible
- Need to lower thresholds

### 6. Common Issues and Solutions

#### Issue: No contours detected
- **Solution**: Lower `canny_low_threshold` and `min_contour_area`

#### Issue: Too many false candidates
- **Solution**: Increase `min_contour_area`, `min_marker_size`, or `min_confidence`

#### Issue: Markers detected but pattern validation fails
- **Solution**: Check `deskewed_marker_with_grid.jpg` - adjust lighting or marker quality

#### Issue: Contours detected but not 4-corners
- **Solution**: The `epsilon_factor` controls polygon approximation precision

## Quick Testing Workflow

1. Run test for 5-10 seconds: `timeout 10s ./test_configurable_detection`
2. Check `debug_frame.jpg` to see what's being detected
3. Adjust parameters in `detection_config.txt`
4. Repeat until satisfied with detection accuracy
5. Use the tuned parameters in your main application

## Example Configuration for Different Scenarios

### High Precision (Few False Positives):
```
min_contour_area=1000
min_marker_size=40
min_confidence=0.8
canny_low_threshold=40
```

### High Sensitivity (Detect Small/Faint Markers):
```
min_contour_area=300
min_marker_size=20
min_confidence=0.5
canny_low_threshold=20
```

### Noisy Environment:
```
blur_kernel_size=3
contrast_alpha=1.5
min_contour_area=800
epsilon_factor=0.03
```
