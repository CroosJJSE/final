# SimpleSegmentation

A simplified segmentation client for processing images and extracting coordinates from segmentation masks. This project demonstrates how to use the CPR library with std::future/promise for asynchronous HTTP requests.

## Features

- Single image segmentation using a remote Flask-based segmentation server
- Both synchronous and asynchronous API for segmentation requests
- Coordinate extraction from segmentation masks
- Example application showcasing both approaches

## Requirements

- CMake 3.14 or later
- C++17 compatible compiler
- OpenCV
- libcurl (will be installed via CPR)
- SSL headers

## Installation

### Ubuntu/Debian

```bash
# Install required packages
sudo apt update
sudo apt install build-essential cmake libssl-dev libcurl4-openssl-dev

# Install OpenCV
sudo apt install libopencv-dev
```

### Project Setup

```bash
# Clone the repository
git clone https://github.com/yourusername/SimpleSegmentation.git
cd SimpleSegmentation

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Install (optional)
sudo make install
```

## Usage

### Basic Usage

```bash
# Run with default image and server URL
./segmentation_client

# Specify image path
./segmentation_client /path/to/image.jpg

# Specify both image path and server URL
./segmentation_client /path/to/image.jpg http://localhost:8000/segment
```

### Example Code

```cpp
#include "SimpleSegmentationClient.h"

// Synchronous example
SimpleSegmentationClient client("http://localhost:8000/segment");
cv::Mat image = cv::imread("input.jpg");
cv::Mat mask = client.segmentImage(image);
std::vector<cv::Point> coordinates = client.extractCoordinatesFromMask(mask);

// Asynchronous example
std::future<cv::Mat> future = client.segmentImageAsync(image);
// Do other work while waiting
cv::Mat asyncMask = future.get();
```

## API Reference

### SimpleSegmentationClient

```cpp
// Constructor
SimpleSegmentationClient(const std::string& server_url);

// Synchronous segmentation
cv::Mat segmentImage(const cv::Mat& image);

// Asynchronous segmentation
std::future<cv::Mat> segmentImageAsync(const cv::Mat& image);

// Extract coordinates from mask
std::vector<cv::Point> extractCoordinatesFromMask(const cv::Mat& mask, int threshold = 127);
```

## Integration with SLAM Pipeline

The coordinate extraction functionality is designed to be integrated with a SLAM pipeline by providing the coordinates of segmented objects. These coordinates can be used to filter out keypoints in the SLAM algorithm that correspond to dynamic objects or other unwanted elements in the scene.

Example integration:

```cpp
// In your SLAM pipeline:
SimpleSegmentationClient segClient("http://server:port/segment");

// Process a frame
cv::Mat frame = getFrameFromCamera();
std::future<cv::Mat> maskFuture = segClient.segmentImageAsync(frame);

// Continue SLAM processing while segmentation happens in background
// ...

// When you need the segmentation results:
cv::Mat mask = maskFuture.get();
std::vector<cv::Point> objectPoints = segClient.extractCoordinatesFromMask(mask);

// Filter keypoints
std::vector<cv::KeyPoint> filteredKeypoints;
for (const auto& kp : originalKeypoints) {
    // Check if the keypoint is in the segmented object
    bool isInObject = false;
    for (const auto& pt : objectPoints) {
        if (std::abs(kp.pt.x - pt.x) < 3 && std::abs(kp.pt.y - pt.y) < 3) {
            isInObject = true;
            break;
        }
    }
    
    // Keep only keypoints that are not part of the segmented object
    if (!isInObject) {
        filteredKeypoints.push_back(kp);
    }
}

// Continue SLAM with filtered keypoints
// ...
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.