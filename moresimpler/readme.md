# YOLOv8 Segmentation Client

This C++ client connects to a YOLOv8 segmentation server to detect and mask humans in images. It's designed to be integrated with SLAM systems to filter out dynamic human objects from keypoint tracking.

## Features

- Asynchronous HTTP requests using CPR library and `std::future`
- Multiple mask handling from YOLOv8 person detections
- Creation of combined masks for all detected humans
- Extraction of pixel coordinates from masks (for SLAM keypoint filtering)
- Visualization of segmentation results

## Prerequisites

- CMake (3.10+)
- C++17 compiler
- OpenCV
- libcurl and SSL headers

For Ubuntu/Debian-based systems:

```bash
sudo apt update
sudo apt install build-essential cmake libcurl4-openssl-dev libssl-dev libopencv-dev
```

## Building the Client

```bash
mkdir build && cd build
cmake ..
make
```

## Running the YOLOv8 Segmentation Server

The server uses YOLOv8 with the segmentation model to detect humans and return segmentation masks.

1. Make sure you have the required Python packages installed:

```bash
pip install flask opencv-python numpy ultralytics torch
```

2. Start the server:

```bash
PYTHONPATH=. python inference_server.py
```

The server will start on `http://0.0.0.0:8000/segment`

## Running the Client

```bash
./yolo_segmenter_client /path/to/your/image.jpg
```

You can also specify a different server URL:

```bash
./yolo_segmenter_client /path/to/your/image.jpg http://server-ip:8000/segment
```

## Output

The client will:

1. Send the image to the server asynchronously
2. Process each human mask received from the server
3. Create a combined mask of all detections
4. Extract pixel coordinates from the mask
5. Display the original image, mask, and visualization
6. Save individual masks as `mask_0.png`, `mask_1.png`, etc.
7. Save the combined mask as `combined_mask.png`

## Integration with SLAM

To integrate with a SLAM system:

1. Pass your frame to the segmentation client
2. Get the pixel coordinates of masked areas
3. Use these coordinates to filter out keypoints in your SLAM pipeline

Example pseudocode for integration:

```cpp
// In your SLAM pipeline
cv::Mat frame = getCurrentFrame();

// Get human masks asynchronously (non-blocking)
auto future_masks = segmenterClient.fetchMasksAsync(frame);

// Continue with other SLAM operations
// ...

// When ready, get the mask and coordinates
std::vector<cv::Mat> masks = future_masks.get();
cv::Mat combinedMask = segmenterClient.createCombinedMask(masks, frame.size());
std::vector<cv::Point> humanPixels = getMaskCoordinates(combinedMask);

// Filter out keypoints that are inside human masks
for (auto& keypoint : keypoints) {
    bool isHuman = false;
    for (const auto& pixel : humanPixels) {
        if (keypoint.pt.x == pixel.x && keypoint.pt.y == pixel.y) {
            isHuman = true;
            break;
        }
    }
    
    if (!isHuman) {
        filteredKeypoints.push_back(keypoint);
    }
}
```

For more efficient checking, you can also use the mask directly:

```cpp
// More efficient way to check if a keypoint is in the mask
for (auto& keypoint : keypoints) {
    int x = static_cast<int>(keypoint.pt.x);
    int y = static_cast<int>(keypoint.pt.y);
    
    // Check if the point is within image bounds
    if (x >= 0 && x < combinedMask.cols && y >= 0 && y < combinedMask.rows) {
        // If the mask value at this point is 0, it's not a human
        if (combinedMask.at<uchar>(y, x) == 0) {
            filteredKeypoints.push_back(keypoint);
        }
    }
}
```

## Troubleshooting

- **Connection Issues**: Check if the server is running and accessible. You can test with curl:
  ```
  curl -X POST -F "image=@test.jpg" http://server-ip:8000/segment
  ```

- **OpenCV Errors**: Ensure OpenCV is correctly installed and linked.

- **Empty Masks**: This could mean no humans were detected in the image, or there's an issue with the YOLOv8 model.